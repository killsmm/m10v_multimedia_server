#ifndef _MEDIA_RECORDER_H
#define _MEDIA_RECORDER_H

extern "C"{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
}
#include <string>
#include <pthread.h>
#include <ctime>
#include "frame_consumer.h"

#define JPEG_PREFIX_STRING "img"
#define JPEG_SUFFIX_STRING ".jpg"

typedef enum {
    RECORD_STATUS_RECORDING,
    RECORD_STATUS_READY,
}RECORD_STATUS;
class MediaRecorder : public FrameConsumer {

    public:
        MediaRecorder(std::string path = ".");
        ~MediaRecorder();
        int start_record(AVCodecID video_codec_id, int width, int height, std::string file_name = "default.mp4"); 
        int stop_record();
        int write_one_frame(uint8_t *addr, unsigned int size);
        AVCodecContext *v_codec;
        AVFormatContext *f_ctx;
        AVPacket packet;
        void onFrameReceivedCallback (void* address, std::uint64_t size, void *extra_data);
        int start();
        int stop();
        AVStream * createGPSStream(AVFormatContext *ctx);
    private:
        pthread_t record_thread;
        std::string record_path;
        int record_thread_index;
        RECORD_STATUS recordStatus;        
        pthread_spinlock_t spinLock;
        bool avi_has_gps_stream;
};




#endif