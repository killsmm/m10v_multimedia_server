#include <iostream>
#include "media_recorder.h"
#include "stream_receiver.h"
#include "jpeg_capture.h"
#include "raw_img_capture.h"
#include "yuv_capture.h"
#include <zmq.hpp>
#include <string>
#include "communicator.h"
#include "json-c/json.h"
#include "rtsp_streamer.h"
#include "live555_server.h"

extern "C" {
     #include "ipcu_stream.h"
    #include "signal.h"
    #include "unistd.h"
}

#define FLAG_DEBUG (1)
#define FLAG_VIDEO (1 << 1)
#define FLAG_MJPEG (1 << 2)
#define FLAG_JPEG (1 << 3)
#define FLAG_RTSP (1 << 4)

static bool app_abort = false;
static char *video_path = NULL;
static char *jpeg_path = NULL;
static char *rtsp_channel_name = NULL;

static std::string PUBLISH_URL = "tcp://*:8888";
static std::string PUBLISH_TOPIC = "jpeg: ";

// static std::string SUB_URL = "tcp://127.0.0.1:8888";
static std::string SUB_URL = "tcp://192.168.137.11:8889";
static std::string SUB_TOPIC = "command: ";

static YuvCapture *yuv_capture = NULL;
static RawImgCapture *raw_capture = NULL;
static JpegCapture *jpeg_capture = NULL;
static MediaRecorder *media_recorder = NULL;
static Communicator *communicator = NULL;
static StreamReceiver *stream_receiver = NULL;
static RtspStreamer *rtsp_streamer = NULL;
static Live555Server *live555_server = NULL;

static void print_help() {
    std::cout << "this is help message" << std::endl;
}

static void signal_handler(int signal) {
    app_abort = true;
}

static void command_handler(std::string cmd){
    std::cout << "cmd: " + cmd << std::endl;
    if (cmd.compare("start") == 0){
        media_recorder = new MediaRecorder(video_path);
        media_recorder->start_record(AV_CODEC_ID_H264, 3840, 2160, "test.avi");
        stream_receiver->addConsumer(E_CPU_IF_COMMAND_STREAM_VIDEO, 0, media_recorder);
    } else if (cmd.compare("stop") == 0){
        if(media_recorder != NULL){
            media_recorder->stop_record();
            communicator->broadcast("record:", "over");
            stream_receiver->removeConsumer(media_recorder);
            delete media_recorder;
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
    while ((c = getopt(argc, argv, "s:mdj:v:")) != -1) {
        switch (c)
        {
        case 'd':
            flag |= FLAG_DEBUG;
            break;
        case 'v':
            flag |= FLAG_VIDEO;
            video_path = optarg;
            break;
        case 'm':
            flag |= FLAG_MJPEG;
            break;
        case 'j':
            flag |= FLAG_JPEG;
            jpeg_path = optarg;
            break;
        case 's':
            flag |= FLAG_RTSP;
            rtsp_channel_name = optarg;
            break;
        default:
            print_help();
            return -1;
        }
    }

    stream_receiver = new StreamReceiver();
    communicator = new Communicator(PUBLISH_URL, SUB_URL);



    if (flag & (FLAG_DEBUG | FLAG_JPEG)) {
        std::cout << "path = " << jpeg_path << std::endl;
        jpeg_capture = new JpegCapture(std::string(jpeg_path));
        stream_receiver->addConsumer(E_CPU_IF_COMMAND_STREAM_JPG, 16, jpeg_capture);
        jpeg_capture->setSavedCallback(savedCallback, static_cast<void *>(communicator));

        raw_capture = new RawImgCapture(std::string(jpeg_path));
        stream_receiver->addConsumer(E_CPU_IF_COMMAND_STREAM_RAW, 1, raw_capture);
        raw_capture->setSavedCallback([](std::string path, void* data){   
                std::cout << "raw saved : " << path << std::endl;
                if(data != NULL){
                    Communicator *comm = static_cast<Communicator *>(data);
                    comm->broadcast("raw: ", path);
                }
            }, static_cast<void *>(communicator));

        yuv_capture = new YuvCapture(std::string(jpeg_path));
        stream_receiver->addConsumer(E_CPU_IF_COMMAND_STREAM_YUV, 1, yuv_capture);
        raw_capture->setSavedCallback([](std::string path, void* data){   
                std::cout << "yuv saved : " << path << std::endl;
                if(data != NULL){
                    Communicator *comm = static_cast<Communicator *>(data);
                    comm->broadcast("yuv: ", path);
                }
            }, static_cast<void *>(communicator));
    }

    if (flag & FLAG_RTSP) {
        system("camera_if_direct 0x1 0x2 0x22\n");
        system("camera_if_direct 0x1 0xc 0xb\n");
        system("camera_if_direct 0x1 0xe 0xb\n");
        system("camera_if_direct 0x1 0xf 0xb\n");
        // system("camera_if_direct 0x1 0x19 0x1\n");
        system("camera_if_direct 0x1 0x41 0xFF01FFFF");
        system("camera_if_direct 0x8 0x3 0x1\n");
        
        system("camera_if_direct 0x0 0xb 0x2\n");
        system("camera_if_direct 0x0 0xb 0x8\n");
        
        /*

        rtsp_streamer = new RtspStreamer(std::string(rtsp_channel_name));
        std::cout << "rtsp channel = " + std::string(rtsp_channel_name) << std::endl;
        stream_receiver->addConsumer(E_CPU_IF_COMMAND_STREAM_VIDEO, 0, rtsp_streamer);
        rtsp_streamer->startPushStream(AV_CODEC_ID_H264, 3840, 2160);
        */
        live555_server = new Live555Server();
        stream_receiver->addConsumer(E_CPU_IF_COMMAND_STREAM_VIDEO, 0, live555_server);
        live555_server->start();
    }
    
    if (flag & FLAG_VIDEO) {
        std::cout << "video path = " << video_path << std::endl;
    }

    stream_receiver->start();

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


    stream_receiver->stop();
    std::cout << "receive stoped" << std::endl;
    return 0;
}
