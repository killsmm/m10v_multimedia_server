#include <iostream>
#include "media_recorder.h"
#include "stream_receiver.h"
#include "jpeg_capture.h"
#include <zmq.hpp>
#include <string>

extern "C" {
    #include <libavformat/avformat.h>
    #include "ipcu_stream.h"
    #include "signal.h"
    #include "unistd.h"
}

#define FLAG_DEBUG (1)
#define FLAG_VIDEO (1 << 1)
#define FLAG_MJPEG (1 << 2)
#define FLAG_JPEG (1 << 3)

static bool app_abort = false;
static char *file_name = NULL;
static char *jpeg_path = NULL;

static const char *PUBLISH_URL = "tcp://*:8888";
static const char *PUBLISH_TOPIC = "jpeg: ";


static void print_help() {
    std::cout << "this is help message" << std::endl;
}

static void signal_handler(int signal) {
    app_abort = true;
}

static void savedCallback(std::string path, void* data){
    if(data != NULL){
        zmq::socket_t *socket = static_cast<zmq::socket_t *>(data);
        std::string msg_str = "";
        msg_str.append(PUBLISH_TOPIC);
        msg_str.append(path);
        zmq::message_t msg(msg_str.c_str(), msg_str.length());
        socket->send(msg, ZMQ_DONTWAIT);
        std::cout << "publish msg " << msg_str << std::endl;

    }
    std::cout << "jpeg saved : " << path << std::endl;
};

int main(int argc, char** argv){
    int flag = 0;
    int c = 0;
    int ret = 0;
    while ((c = getopt(argc, argv, "mdj:v:")) != -1) {
        switch (c)
        {
        case 'd':
            flag |= FLAG_DEBUG;
            break;
        case 'v':
            flag |= FLAG_VIDEO;
            file_name = optarg;
            break;
        case 'm':
            flag |= FLAG_MJPEG;
            break;
        case 'j':
            flag |= FLAG_JPEG;
            jpeg_path = optarg;
            break;
        default:
            print_help();
        }
    }

    StreamReceiver receiver;

    zmq::context_t ctx;
    zmq::socket_t publisher(ctx, zmq::socket_type::pub);
    publisher.bind(PUBLISH_URL);

    std::cout << "path = " << jpeg_path << std::endl;

    if (flag & (FLAG_DEBUG | FLAG_JPEG)) {
        JpegCapture jpegCapture(16, std::string(jpeg_path));
        receiver.addConsumer(&jpegCapture);
        jpegCapture.setSavedCallback(savedCallback, (void*)&publisher);
    }else if (flag & FLAG_VIDEO) {
        MediaRecorder recorder;
        receiver.addConsumer(&recorder);
    }

    receiver.start();

    signal(SIGINT, signal_handler);    
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGABRT, signal_handler);

    while (!app_abort)
    {
        sleep(1);
    }
    receiver.stop();
    std::cout << "receive stoped" << std::endl;
    return 0;
}
