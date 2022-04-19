#include <iostream>
#include "media_recorder.h"
#include "stream_receiver.h"
#include "jpeg_capture.h"
#include <zmq.hpp>
#include <string>
#include "communicator.h"
#include "json-c/json.h"

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

static std::string PUBLISH_URL = "tcp://*:8888";
static std::string PUBLISH_TOPIC = "jpeg: ";

// static std::string SUB_URL = "tcp://127.0.0.1:8888";
static std::string SUB_URL = "tcp://192.168.137.11:8889";
static std::string SUB_TOPIC = "command: ";

static JpegCapture *jpeg_capture = NULL;
static MediaRecorder *media_recorder = NULL;
static Communicator *communicator = NULL;

static void print_help() {
    std::cout << "this is help message" << std::endl;
}

static void signal_handler(int signal) {
    app_abort = true;
}

static void command_handler(std::string cmd){
    std::cout << "cmd: " + cmd << std::endl;
    if (cmd.compare("start") == 0){
        if(media_recorder != NULL){
            media_recorder->start_record(AV_CODEC_ID_H265, 1920, 1080);
        }
    } else if (cmd.compare("stop") == 0){
        if(media_recorder != NULL){
            media_recorder->stop_record();
            communicator->broadcast("record:", "over");
        }
    }

}

static void savedCallback(std::string path, void* data){
    std::cout << "jpeg saved : " << path << std::endl;
    if(data != NULL){
        Communicator *comm = static_cast<Communicator *>(data);
        comm->broadcast(PUBLISH_TOPIC, path);

        // std::string msg_str = "";
        // msg_str.append(PUBLISH_TOPIC);
        // msg_str.append(path);
        // zmq::message_t msg(msg_str.c_str(), msg_str.length());
        // socket->send(msg, ZMQ_DONTWAIT);
    }
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
    communicator = new Communicator(PUBLISH_URL, SUB_URL);


    if (flag & (FLAG_DEBUG | FLAG_JPEG)) {
        std::cout << "path = " << jpeg_path << std::endl;
        jpeg_capture = new JpegCapture(std::string(jpeg_path));
        receiver.addConsumer(16, jpeg_capture);
        jpeg_capture->setSavedCallback(savedCallback, static_cast<void *>(communicator));
    }
    
    if (flag & FLAG_VIDEO) {
        std::cout << "video path = " << file_name << std::endl;
        media_recorder = new MediaRecorder(file_name);
        media_recorder->init();
        receiver.addConsumer(8, media_recorder);
    }

    receiver.start();

    signal(SIGINT, signal_handler);    
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGABRT, signal_handler);

    std::string received_msg;
    while (!app_abort)
    {
        if(communicator->receive(SUB_TOPIC, received_msg)){
            std::string command = received_msg.substr(SUB_TOPIC.size(), received_msg.size() - SUB_TOPIC.size());
            std::cout << command << std::endl;
            command_handler(command);
        }
        sleep(1);
    }


    receiver.stop();
    std::cout << "receive stoped" << std::endl;
    return 0;
}
