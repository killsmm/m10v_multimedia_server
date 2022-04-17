#include "media_recorder.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <fcntl.h>

extern "C"{
#include "libavformat/avformat.h"
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

MediaRecorder::MediaRecorder(int stream_id, std::string file_path) : FrameConsumer(stream_id){
    pthread_spin_init(&spinLock, PTHREAD_PROCESS_PRIVATE);
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
        return ret;
    }

    av_init_packet(&this->packet);
    uint8_t *data =  (uint8_t *)av_malloc(4);
    data[0] = 0x2;
    data[1] = 0x3;
    data[2] = 0x4;
    data[3] = 0x5;
    ret = av_packet_from_data(&this->packet, data, 4);
    if(ret != 0){
        std::cout << "av packet data from data failed" << std::endl;
        return -1;
    }

    this->packet.stream_index = this->f_ctx->streams[1]->index;
    this->packet.pts = time_stamp;
    this->packet.dts = time_stamp;

    ret = av_write_frame(this->f_ctx, &this->packet);
    av_packet_unref(&this->packet);
    return ret;
}

int MediaRecorder::init() {
    int ret = 0;
    av_register_all();
    ret = avformat_alloc_output_context2(&this->f_ctx, NULL, NULL, file_path.c_str());
    if(ret != 0){
        std::cerr << "avformat_alloc_output_context2 ret = " << ret << std::endl;
        return ret;
    }
    av_dump_format(this->f_ctx, 0, this->file_path.c_str(), 1);
    std::cout << this->f_ctx->oformat->long_name << std::endl;
    printf("register succeed\n");
    return 0;
}

int MediaRecorder::set_file_path(std::string file_path) {
    this->file_path = file_path;
}

std::string MediaRecorder::get_file_path() {
    return this->file_path;
}

int MediaRecorder::start_record(AVCodecID video_codec_id, int width, int height) {
    int ret = 0;
    ret = avio_open(&this->f_ctx->pb, this->file_path.c_str(), AVIO_FLAG_WRITE);
    if(ret != 0){
        std::cerr << "avio_open ret = " << ret  << std::endl;
        return ret;
    }

    AVStream *stream = avformat_new_stream(this->f_ctx, NULL);
    if( stream == NULL){
        std::cerr << "avformat_new_stream failed" << std::endl;
        return -1;
    }

    stream->codecpar->codec_id = video_codec_id;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->codecpar->width = width;
    stream->codecpar->height = height;

    AVStream *gps_stream = avformat_new_stream(this->f_ctx, NULL);
    gps_stream->codecpar->codec_type = AVMEDIA_TYPE_DATA;
    gps_stream->codecpar->codec_id = AV_CODEC_ID_TTF;

    ftime(&start_time);

    ret = avformat_write_header(this->f_ctx, NULL);
    printf("return = %d\n", ret);
    if(ret != 0){
        std::cerr << "avformat_write_header failed" << std::endl;
        return -1;
    }
    status = RECORD_STATUS_RECORDING;
    return ret;
}

int MediaRecorder::stop_record() {
    status = RECORD_STATUS_READY;
    pthread_spin_lock(&spinLock);
    av_write_trailer(this->f_ctx);
    pthread_spin_unlock(&spinLock);
}

void MediaRecorder::onFrameReceivedCallback(void* address, std::uint64_t size) {
    pthread_spin_lock(&spinLock);
    if(status == RECORD_STATUS_RECORDING){
        write_one_frame(static_cast<uint8_t *>(address), size);
    }
    pthread_spin_unlock(&spinLock);
}

int MediaRecorder::start() {
    
}

int MediaRecorder::stop() {
    
}
