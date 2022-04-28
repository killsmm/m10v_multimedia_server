#include "rtsp_streamer.h"
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


RtspStreamer::RtspStreamer(std::string stream_name) {
    pthread_spin_init(&spinLock, PTHREAD_PROCESS_PRIVATE);
    this->streamName = stream_name;
    this->rtspStatus = RTSP_STATUS_READY;

    av_register_all();
    avformat_network_init();
}

RtspStreamer::~RtspStreamer() {
    
}

int RtspStreamer::write_one_frame(uint8_t *addr, unsigned int size) {
    av_init_packet(&this->packet);
	int ret = av_packet_from_data(&this->packet, addr, size);
    if(ret != 0){
        std::cout << "av packet from data failed" << std::endl;
        return -1;
    }
    this->packet.stream_index = this->f_ctx->streams[0]->index;
    printf("time base = %d/%d\n", this->f_ctx->streams[0]->time_base.num, this->f_ctx->streams[0]->time_base.den);
    int64_t time_stamp = get_timestamp(this->f_ctx->streams[0]->time_base);
    this->packet.pts = time_stamp;
    this->packet.dts = time_stamp;
    this->packet.flags = AVINDEX_KEYFRAME;
    av_pkt_dump2(stdout, &this->packet, 0, this->f_ctx->streams[0]);
    // ret = av_interleaved_write_frame(this->f_ctx, &this->packet);
    printf("write frame start\n");
    ret = av_write_frame(this->f_ctx, &this->packet);

    printf("write frame end\n");
    if (ret != 0) {
        perror("write frame");
        std::cout << "write frame failed\n" << std::endl;
    }

    return ret;
}

void RtspStreamer::onFrameReceivedCallback(void* address, std::uint64_t size) {
    pthread_spin_lock(&spinLock);
    // printf("MediaRecorder::onFrameReceivedCallback\n");
    if(this->rtspStatus == RTSP_STATUS_PUSHING){
        std::cout << "-";
        write_one_frame(static_cast<uint8_t *>(address), size);
    }
    pthread_spin_unlock(&spinLock);
}

int RtspStreamer::startPushStream(AVCodecID video_codec_id, int width, int height) {
    int ret = 0;
    std::string full_path = DEFAULT_RTSP_ADDRESS + this->streamName;
    std::cout << "full path:" + full_path << std::endl;
    ret = avformat_alloc_output_context2(&this->f_ctx, NULL, "rtsp", full_path.c_str());
    // ret = avformat_alloc_output_context2(&this->f_ctx, NULL, NULL, "test.mp4");
    if(ret != 0){
        std::cerr << "avformat_alloc_output_context2 ret = " << ret << std::endl;
        return ret;
    }

    this->f_ctx->max_interleave_delta = 1000000;
    std::cout << this->f_ctx->oformat->long_name << std::endl;



    printf("pb = %d\n", this->f_ctx->pb);
    printf("flags = %08x\n", this->f_ctx->flags);

    

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

    // stream->codec->codec_id = video_codec_id;
    // stream->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    // stream->codec->width = width;
    // stream->codec->height = height;

    // ret = avio_open2(&this->f_ctx->pb, "rtsp://test:password@192.168.33.19:1935/ffmpeg/0", AVIO_FLAG_WRITE, NULL,NULL);
    // if(ret < 0){
    //     perror("avio_open failed\n");
    //     std::cerr << "avio_open ret = " << ret  << std::endl;
    //     return ret;
    // }

    
    ftime(&start_time);

	AVDictionary* options = NULL;
	av_dict_set(&options, "buffer_size", "1024000", 0); //设置最大缓存，1080可调到最大
	av_dict_set(&options, "rtsp_transport", "tcp", 0); //以tcp的方式传送
	av_dict_set(&options, "stimeout", "5000000", 0); //设置超时断开链接时间，单位us
	av_dict_set(&options, "max_delay", "500000", 0); //设置最大时延
    av_dict_set(&options, "rtsp_flags", "listen", 0); //设置最大时延
	// av_dict_set(&options, "framerate", "20", 0); 

    ret = avformat_write_header(this->f_ctx, &options);
    
    printf("write rtsp header return = %d\n", ret);
    if(ret != 0){
        std::cerr << "avformat_write_header failed" << std::endl;
        return -1;
    }
    uint8_t data[50000];
    while(1){
        write_one_frame(data,8000);
    }

    pthread_spin_lock(&spinLock);
    rtspStatus = RTSP_STATUS_PUSHING;
    pthread_spin_unlock(&spinLock);
    return ret;
}

int RtspStreamer::stopPushStream() {
    
}
