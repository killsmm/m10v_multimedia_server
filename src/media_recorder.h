#ifndef _MEDIA_RECORDER_H
#define _MEDIA_RECORDER_H

extern "C"{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
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
        MediaRecorder(int stream_id = 0, std::string file_path = "");
        ~MediaRecorder();
        int init();
        int set_file_path(std::string file_path);
        std::string get_file_path();
        int start_record(AVCodecID video_codec_id, int width, int height);
        int stop_record();
        int write_one_frame(uint8_t *addr, unsigned int size);
        AVCodecContext *v_codec;
        AVFormatContext *f_ctx;
        AVPacket packet;
        void onFrameReceivedCallback (void* address, std::uint64_t size);
        int start();
        int stop();
    private:
        pthread_t record_thread;
        std::string file_path;
        int record_thread_index;
        RECORD_STATUS status;        
        pthread_spinlock_t spinLock;
};




#endif