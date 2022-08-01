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
#include "sei_encoder.h"
#include <regex>
#include "device_status.h"

//#define RTSP_TEST
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

static std::string getValueFromJson(json_object *json, std::string level1, std::string level2, std::string level3){
    json_object *tmp1;
    json_object *tmp2;
    if(!json_object_object_get_ex(json, level1.data(), &tmp1)){
        std::cout << "cannot find cmd" << std::endl;
        return "";
    }

    if(level2 == ""){
        const char *result = json_object_get_string(tmp1);
        return std::string(result, strlen(result));
    }

    if(!json_object_object_get_ex(tmp1, level2.data(), &tmp2)){
        return "";
    }

    if(level3 == ""){
        const char *result = json_object_get_string(tmp2);
        return std::string(result, strlen(result));
    }

    if(!json_object_object_get_ex(tmp2, level3.data(), &tmp1)){
        return "";
    }

    const char *result = json_object_get_string(tmp1);
    return std::string(result, strlen(result));
}

static float getNumberFromJson(json_object *json, std::string level1, std::string level2, std::string level3){
    json_object *tmp1;
    json_object *tmp2;
    if(!json_object_object_get_ex(json, level1.data(), &tmp1)){
        std::cout << "cannot find cmd" << std::endl;
        return 0;
    }

    if(level2 == ""){
        return json_object_get_double(tmp1);
    }

    if(!json_object_object_get_ex(tmp1, level2.data(), &tmp2)){
        return 0;
    }

    if(level3 == ""){
        return json_object_get_double(tmp2);
    }

    if(!json_object_object_get_ex(tmp2, level3.data(), &tmp1)){
        return 0;
    }

    return json_object_get_double(tmp1);
}

static void savedCallback(std::string path, void* data){
    std::cout << "jpeg saved : " << path << std::endl;
    if(data != NULL){
        Communicator *comm = static_cast<Communicator *>(data);
        comm->broadcast ("", "{\"cmd\":\"TakePhotoResult\",\"data\":{\"path\":\"/mnt/temp\",\"result\":\"success\"}}");
        comm->broadcast("http://192.168.137.16:8081/", path);
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
    communicator = new Communicator();

    SeiEncoder::init();

    char serial_number[20] = {0};
    FILE *fp = popen("cat /sys/class/net/eth0/address", "r");
    if(fp != NULL){
        fgets(serial_number, sizeof(serial_number), fp);
    }
    fclose(fp);
    DeviceStatus::serial_number.assign(serial_number);

    if (flag & FLAG_JPEG) {
        std::cout << "parent path = " << jpeg_path << std::endl;
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
        yuv_capture->setSavedCallback([](std::string path, void* data){   
                std::cout << "yuv saved : " << path << std::endl;
                if(data != NULL){
                    Communicator *comm = static_cast<Communicator *>(data);
                    comm->broadcast("yuv: ", path);
                }
            }, static_cast<void *>(communicator));
    }

    if (flag & FLAG_RTSP) {
#ifdef RTSP_TEST
        system("camera_if_direct 0x1 0x2 0x22\n");
        system("camera_if_direct 0x1 0xc 0xb\n");
        system("camera_if_direct 0x1 0xe 0xb\n");
        system("camera_if_direct 0x1 0xf 0xb\n");
        system("camera_if_direct 0x1 0x41 0xFF01FFFF");
        system("camera_if_direct 0x8 0x3 0x1\n");
        system("camera_if_direct 0x0 0xb 0x2\n");
        system("camera_if_direct 0x0 0xb 0x8\n");
#endif
        live555_server = new Live555Server(std::string(rtsp_channel_name));
        stream_receiver->addConsumer(E_CPU_IF_COMMAND_STREAM_VIDEO, 0, live555_server);
        live555_server->start();
    }
    
    if (flag & FLAG_VIDEO) {
        std::cout << "video path = " << video_path << std::endl;
        media_recorder = new MediaRecorder(video_path);
    }

    stream_receiver->start();

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGABRT, signal_handler);

    std::string received_msg;
    while (!app_abort)
    {
        // SeiEncoder::setLocation(rand() % 90, rand() % 90, rand() % 3000); // for test only
        if(communicator->receiveSub(received_msg)){
            json_tokener *tok = json_tokener_new();
            json_object *json = json_tokener_parse_ex(tok, received_msg.data(), received_msg.size());
            std::string cmd = getValueFromJson(json, "cmd", "", "");
            if (cmd == ""){
                std::cout << json_object_to_json_string(json) << std::endl;
            }

            if(cmd == "GPS"){
#if 0
                float latitude = std::stof(getValueFromJson(json, "data", "location", "latitude"));
                float longitude = std::stof(getValueFromJson(json, "data", "location", "longitude"));
                float altitude = std::stof(getValueFromJson(json, "data", "location", "altitude"));
                float roll = std::stof(getValueFromJson(json, "data", "angles", "roll"));
                float pitch = std::stof(getValueFromJson(json, "data", "angles", "pitch"));
                float yaw = std::stof(getValueFromJson(json, "data", "angles", "yaw"));
                printf("GPS: (%f,%f,%f), (%f,%f,%f)\n", latitude, longitude, altitude, roll, pitch, yaw);
#endif
                SeiEncoder::setLocation(std::stof(getValueFromJson(json, "data", "location", "latitude")),
                                        std::stof(getValueFromJson(json, "data", "location", "longitude")),
                                        std::stof(getValueFromJson(json, "data", "location", "altitude")));
                SeiEncoder::setAngles(std::stof(getValueFromJson(json, "data", "angles", "roll")),
                                      std::stof(getValueFromJson(json, "data", "angles", "pitch")),
                                      std::stof(getValueFromJson(json, "data", "angles", "yaw"))); 
            }else if(cmd == "DeviceStatus"){
                DeviceStatus::cmos_temperature = std::stof(getValueFromJson(json, "data", "cmos_temp", ""));
                DeviceStatus::input_voltage = std::stof(getValueFromJson(json, "data", "total_voltage", ""));
            }else if(cmd == "workStatus"){
                DeviceStatus::shutter_mode = getNumberFromJson(json, "data", "shutter_mode", "");
                std::cout << "shutter_mode = " << DeviceStatus::shutter_mode << std::endl;
                std::string nr_str = getValueFromJson(json, "data", "noise_reduction_strength", "");
                if(nr_str != ""){
                    DeviceStatus::noise_reduction_strength = std::stoi(nr_str);
                }
                std::string jq_str = getValueFromJson(json, "data", "photo_resolve", "");
                if(jq_str != ""){
                    uint32_t jpeg_quality = std::stoi(jq_str);
                    if(jpeg_quality == 100){
                        DeviceStatus::jpeg_quality_level = 2;
                    }else if(jpeg_quality > 90 && jpeg_quality < 100){
                        DeviceStatus::jpeg_quality_level = 1;
                    }else{
                        DeviceStatus::jpeg_quality_level = 0;
                    }
                }

                std::string sc_str = getValueFromJson(json, "data", "shutter_count", "count1");
                if(sc_str != ""){
                    DeviceStatus::shutter_count = std::stoi(sc_str);
                }
            }
            json_object_put(json);
            json_tokener_free(tok);
        }

        communicator->receiveCmd([](std::string s) -> bool{
            json_tokener *tok = json_tokener_new();
            json_object *json = json_tokener_parse_ex(tok, s.data(), s.size());
            std::string cmd = getValueFromJson(json, "cmd", "", "");
            std::cout << "---cmd from request:" + cmd << std::endl;
            if(cmd == "VideoStorage"){
                media_recorder->setPath(getValueFromJson(json, "data", "path", ""));
                media_recorder->setPrefix(getValueFromJson(json, "data", "prefix", ""));
            }else if(cmd == "PhotoStorage"){
                if(jpeg_capture != NULL){
                    jpeg_capture->setSubPath(getValueFromJson(json, "data", "path", ""));
                    jpeg_capture->setPrefix(getValueFromJson(json, "data", "prefix", ""));
                }
            }else if(cmd == "VideoStart"){
                int width = 1920;
                int height = 1080;
                int stream_id = 0;
                AVCodecID codec_id = AV_CODEC_ID_H264;
                std::string resolution = getValueFromJson(json, "data", "resolve", "");
                std::cout << "resolution " << resolution << std::endl;
                if(resolution != ""){
                    std::regex re("(\\d+)x(\\d+)");
                    std::smatch match;
                    std::regex_match(resolution, match, re);
                    if(match.size() == 3){
                        width =  std::stoi(match.str(1));
                        height = std::stoi(match.str(2)); 
                    }
                }
                std::string stream_type = getValueFromJson(json, "data", "format", "");
                std::cout << "stream_type " << stream_type << std::endl;
                if(stream_type == "H264_0"){
                    stream_id = 0;
                    codec_id = AV_CODEC_ID_H264;
                }else if(stream_type == "H264_1"){
                    stream_id = 1;
                    codec_id = AV_CODEC_ID_H264;
                }else if(stream_type == "H264_2"){
                    stream_id = 2;
                    codec_id = AV_CODEC_ID_H264;
                }else if(stream_type == "H265"){
                    stream_id = 8;
                    codec_id = AV_CODEC_ID_H265;
                }else{
                    stream_id = 0;
                    codec_id = AV_CODEC_ID_H264;
                }

                int frame_rate = 25;
                std::string fps = getValueFromJson(json, "data", "fps", "");
                if (fps != ""){
                    frame_rate = std::stoi(fps);
                }
                

                std::cout << "width, height = " << width << "," << height << std::endl;
                std::cout << "codec_id = " << codec_id << " stream_id == " << stream_id << std::endl;
                std::cout << "frame_rate = " << frame_rate << std::endl;

                if(media_recorder->getRecordStatus() == RECORD_STATUS_RECORDING){
                    std::cout << "error: is already recording" << std::endl;
                    json_object_put(json);
                    json_tokener_free(tok);
                    return false;
                }
                media_recorder->start_record(codec_id, width, height, frame_rate, ".avi");
                stream_receiver->addConsumer(E_CPU_IF_COMMAND_STREAM_VIDEO, stream_id, media_recorder);
            }else if(cmd == "VideoStop"){
                if(media_recorder->getRecordStatus() != RECORD_STATUS_RECORDING){
                    std::cout << "error: is not recording" << std::endl;
                    json_object_put(json);
                    json_tokener_free(tok);
                    return false;
                }
                media_recorder->stop_record();
                communicator->broadcast("record:", "over");
                stream_receiver->removeConsumer(media_recorder);
            }else{
                json_object_put(json);
                json_tokener_free(tok);
                return false;
            }
            json_object_put(json);
            json_tokener_free(tok);
            return true;
        });

        //sleep(1);
    }


    stream_receiver->stop();
    std::cout << "receive stoped" << std::endl;
    return 0;
}
