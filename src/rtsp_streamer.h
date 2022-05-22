#ifndef RTSP_STREAMER_H
#define RTSP_STREAMER_H

extern "C"{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
}
#include <string>
#include <pthread.h>
#include <ctime>
#include "frame_consumer.h"

#define DEFAULT_RTSP_ADDRESS "rtsp://localhost:8554/"

typedef enum {
    RTSP_STATUS_PUSHING,
    RTSP_STATUS_READY,
}RTSP_STATUS;

class RtspStreamer : public FrameConsumer {

    public:
        RtspStreamer(std::string stream_name = "test");
        ~RtspStreamer();
        int startPushStream(AVCodecID video_codec_id, int width, int height);
        int stopPushStream();
        int write_one_frame(uint8_t *addr, unsigned int size);
        void onFrameReceivedCallback (void* address, std::uint64_t size);
    private:
        AVCodecContext *v_codec;
        AVFormatContext *f_ctx;
        AVPacket packet;
        std::string streamName;
        pthread_spinlock_t spinLock;
        RTSP_STATUS rtspStatus;
};




#endif