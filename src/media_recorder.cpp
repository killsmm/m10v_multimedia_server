#include "media_recorder.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <fcntl.h>
#include "space_data_reporter.h"

extern "C"{
// #include "libavformat/avformat.h"
#include "cif_stream.h"
#include "ipcu_stream.h"
}

static timeb start_time;

static int get_timestamp(AVRational time_base){
    timeb t;
    ftime(&t);
    int64_t time_stamp_ms = ((int64_t)t.time * 1000 + t.millitm) - ((int64_t)start_time.time * 1000 + start_time.millitm);
    return av_rescale_q(time_stamp_ms, (AVRational){1, 1000}, time_base);
}

MediaRecorder::MediaRecorder(std::string path){
    pthread_spin_init(&spinLock, PTHREAD_PROCESS_PRIVATE);
    this->record_path = path;
    this->avi_has_gps_stream = true;
    av_register_all();
}

MediaRecorder::~MediaRecorder() {
    
}

int MediaRecorder::write_one_frame(uint8_t *addr, unsigned int size) {
    av_init_packet(&this->packet);

	int ret = av_packet_from_data(&this->packet, addr, size);
    if(ret != 0){
        std::cout << "av packet from data failed" << std::endl;
        return -1;
    }

    this->packet.stream_index = this->f_ctx->streams[0]->index;
    int64_t time_stamp = get_timestamp(this->f_ctx->streams[0]->time_base);
    this->packet.pts = time_stamp;
    this->packet.dts = time_stamp;
    
    ret = av_write_frame(this->f_ctx, &this->packet);
    if (ret != 0) {
        std::cout << "write frame failed\n" << std::endl;
    }

    if (this->avi_has_gps_stream){
        uint8_t space_data[SpaceDataReporter::SPACE_DATA_SIZE]; 
        SpaceDataReporter::writeCurrentSpaceDataToBuf(space_data);
        av_packet_from_data(&this->packet, space_data, sizeof(space_data));
        this->packet.stream_index = this->f_ctx->streams[1]->index;
        int64_t time_stamp = get_timestamp(this->f_ctx->streams[1]->time_base);
        this->packet.pts = time_stamp;
        this->packet.dts = time_stamp;
        ret |= av_write_frame(this->f_ctx, &this->packet);
        if (ret != 0){
            std::cout << "write gps frame failed" << std::endl;
        }
    }

    return ret;
}

int MediaRecorder::start_record(AVCodecID video_codec_id, int width, int height, std::string file_name) {
    int ret = 0;
    std::string full_path = this->record_path + "/" + file_name;
    std::cout << "full path:" + full_path << std::endl;
    ret = avformat_alloc_output_context2(&this->f_ctx, NULL, NULL, full_path.c_str());
    if(ret != 0){
        std::cerr << "avformat_alloc_output_context2 ret = " << ret << std::endl;
        return ret;
    }
    av_dump_format(this->f_ctx, 0, full_path.c_str(), 1);
    std::cout << this->f_ctx->oformat->long_name << std::endl;

    ret = avio_open(&this->f_ctx->pb, full_path.c_str(), AVIO_FLAG_WRITE);
    if(ret != 0){
        std::cerr << "avio_open ret = " << ret  << std::endl;
        return ret;
    }

    AVStream *stream = avformat_new_stream(this->f_ctx, NULL);
    if( stream == NULL){
        std::cerr << "avformat_new_stream failed" << std::endl;
        return -1;
    }
    std::cout << "new stream index = " << stream->index << std::endl;

    stream->codecpar->codec_id = video_codec_id;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->codecpar->width = width;
    stream->codecpar->height = height;

    if (this->avi_has_gps_stream){
        AVStream *gps_stream = createGPSStream(this->f_ctx);
    }

    ftime(&start_time);

    ret = avformat_write_header(this->f_ctx, NULL);
    printf("return = %d\n", ret);
    if(ret != 0){
        std::cerr << "avformat_write_header failed" << std::endl;
        return -1;
    }
    pthread_spin_lock(&spinLock);
    recordStatus = RECORD_STATUS_RECORDING;
    pthread_spin_unlock(&spinLock);
    return ret;
}

int MediaRecorder::stop_record() {
    recordStatus = RECORD_STATUS_READY;
    pthread_spin_lock(&spinLock);
    av_write_trailer(this->f_ctx);
    avformat_free_context(this->f_ctx);
    pthread_spin_unlock(&spinLock);
    return 0;
}

void MediaRecorder::onFrameReceivedCallback(void* address, std::uint64_t size) {
    pthread_spin_lock(&spinLock);
    // printf("MediaRecorder::onFrameReceivedCallback\n");
    if(recordStatus == RECORD_STATUS_RECORDING){
        printf("-");
        write_one_frame(static_cast<uint8_t *>(address), size);
    }
    pthread_spin_unlock(&spinLock);
}

AVStream * MediaRecorder::createGPSStream(AVFormatContext *ctx){
    AVStream *stream = avformat_new_stream(ctx, NULL);
    stream->codecpar->codec_id = AV_CODEC_ID_BIN_DATA;
    stream->codecpar->codec_type = AVMEDIA_TYPE_DATA;
    stream->codecpar->sample_rate = 30;
    stream->codecpar->bits_per_raw_sample = 32 * 8;
    return stream;
}