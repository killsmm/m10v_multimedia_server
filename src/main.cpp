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
#include "gpio_ctrl.h"
#include "boost/program_options.hpp"
#include "configs.h"

//#define RTSP_TEST
extern "C" {
     #include "ipcu_stream.h"
    #include "signal.h"
    #include "unistd.h"
}

#define FEEDBACK_GPIO 203

#define FLAG_DEBUG (1)
#define FLAG_VIDEO (1 << 1)
#define FLAG_MJPEG (1 << 2)
#define FLAG_JPEG (1 << 3)
#define FLAG_RTSP (1 << 4)

static bool app_abort = false;

static std::string video_path = DEFAULT_VIDOE_RECORD_PATH;
static std::string jpeg_path = DEFUALT_JPEG_PATH;
static std::string rtsp_channel_name = DEFAULT_RTSP_CHANNEL_NAME;
static std::string ram_dcim_path = DEFAULT_RAM_DCIM_PATH;
static std::string ram_dcim_url = "http://";



static YuvCapture *yuv_capture = NULL;
static RawImgCapture *raw_capture = NULL;
static JpegCapture *jpeg_capture = NULL;
static MediaRecorder *media_recorder = NULL;
static Communicator *communicator = NULL;
static StreamReceiver *stream_receiver = NULL;
static Live555Server *live555_server = NULL;



static void signal_handler(int signal) {
    app_abort = true;
}

static std::string getStrFromJson(json_object *json, std::string level1, std::string level2, std::string level3){
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

static float getFloatFromJson(json_object *json, std::string level1, std::string level2, std::string level3){
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
        comm->broadcast(std::string(ram_dcim_url), path);
    }
};


namespace po = boost::program_options;

static int string_to_float(const char* str, float *result){
    char *tmp;
    *result = std::strtof(str, &tmp);
    if(tmp == str){
        return -1;
    }
    return 0;
}

static int validate_gps(float latitude, float longitude, float altitude, float roll, float pitch, float yaw){
    if (latitude < -90.0 || latitude > 90.0 || abs(latitude) < 0.1f){
        return -1;
    }
    if (longitude < -180.0 || longitude > 180.0 || abs(longitude) < 0.1f){
        return -1;
    }
    if (altitude < -1000.0 || altitude > 10000.0 || abs(altitude) < 0.1f){
        return -1;
    }
    if (roll < -180.0 || roll > 180.0 || abs(roll) < 0.1f){
        return -1;
    }
    if (pitch < -180.0 || pitch > 180.0 || abs(pitch) < 0.1f){
        return -1;
    }
    if (yaw < -180.0 || yaw > 180.0 || abs(yaw) < 0.1f){
        return -1;
    }
    return 0;
}

static void handle_sub_msg(std::string msg){
    json_tokener *tok = json_tokener_new();
    json_object *json = json_tokener_parse_ex(tok, msg.data(), msg.size());
    std::string cmd = getStrFromJson(json, "cmd", "", "");
    if (cmd == ""){
        std::cout << json_object_to_json_string(json) << std::endl;
    }

    if(cmd == "GPS"){
        char **tmp;
        float latitude = 0;
        float longitude = 0;
        float altitude = 0;
        float roll = 0;
        float pitch = 0;
        float yaw = 0;
        
        if(string_to_float(getStrFromJson(json, "data", "location", "latitude").data(), &latitude) != 0){
            return;
        }
        if(string_to_float(getStrFromJson(json, "data", "location", "longitude").data(), &longitude) != 0){
            return;
        }
        if(string_to_float(getStrFromJson(json, "data", "location", "altitude").data(), &altitude) != 0){
            return;
        }
        if(string_to_float(getStrFromJson(json, "data", "angles", "roll").data(), &roll) != 0){
            return;
        }
        if(string_to_float(getStrFromJson(json, "data", "angles", "pitch").data(), &pitch) != 0){
            return;
        }
        if(string_to_float(getStrFromJson(json, "data", "angles", "yaw").data(), &yaw) != 0){
            return;
        }
        if (latitude < -90.0 || latitude > 90.0 || abs(latitude) < 0.1f){
            return;
        }
        if (longitude < -180.0 || longitude > 180.0 || abs(longitude) < 0.1f){
            return;
        }

        if(validate_gps(latitude, longitude, altitude, roll, pitch, yaw) != 0){
            return;
        }
        SeiEncoder::setLocation(latitude, longitude, altitude);
        SeiEncoder::setAngles(roll, pitch, yaw);
        SeiEncoder::time_stamp = std::stol(getStrFromJson(json, "data", "time_stamp", ""));
    }else if(cmd == "DeviceStatus"){
        DeviceStatus::cmos_temperature = std::stof(getStrFromJson(json, "data", "cmos_temp", ""));
        DeviceStatus::input_voltage = std::stof(getStrFromJson(json, "data", "total_voltage", ""));
    }else if(cmd == "workStatus"){
        DeviceStatus::shutter_mode = getFloatFromJson(json, "data", "shutter_mode", "");
        std::cout << "shutter_mode = " << DeviceStatus::shutter_mode << std::endl;
        std::string nr_str = getStrFromJson(json, "data", "noise_reduction_strength", "");
        if(nr_str != ""){
            DeviceStatus::noise_reduction_strength = std::stoi(nr_str);
        }
        std::string jq_str = getStrFromJson(json, "data", "photo_resolve", "");
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

        std::string sc_str = getStrFromJson(json, "data", "shutter_count", "count1");
        if(sc_str != ""){
            DeviceStatus::shutter_count = std::stoi(sc_str);
        }
    }
    json_object_put(json);
    json_tokener_free(tok);
}
/**
 * @brief 
 * 
 * @param cmd VideoStorage, PhotoStorage, VideoStart, VideoStop
 * @return true 
 * @return false 
 */
static bool handle_cmd(std::string cmd_string){
    json_tokener *tok = json_tokener_new();
    json_object *json = json_tokener_parse_ex(tok, cmd_string.data(), cmd_string.size());
    std::string cmd = getStrFromJson(json, "cmd", "", "");
    std::cout << "---cmd from request:" + cmd << std::endl;
    if(cmd == "VideoStorage"){
        if(media_recorder == NULL){
            return false;
        }
        media_recorder->setPath(getStrFromJson(json, "data", "path", ""));
        media_recorder->setPrefix(getStrFromJson(json, "data", "prefix", ""));
    }else if(cmd == "PhotoStorage"){
        if(jpeg_capture != NULL){
            std::string p = getStrFromJson(json, "data", "path", "");
            std::cout << "###set path " + p << std::endl;
            std::string sub_str = "/run/SD/mmcblk0p1/DCIM";
            size_t found = p.find(sub_str);
            if(found != std::string::npos){
                std::cout << "found parent path" << std::endl;
                p.erase(found, found + sub_str.length());
                std::cout << "#### final path = " + p << std::endl;
            }
            jpeg_capture->setSubPath(p);
            jpeg_capture->setPrefix(getStrFromJson(json, "data", "prefix", ""));
        }
    }else if(cmd == "VideoStart"){
        int width = 1920;
        int height = 1080;
        int stream_id = 0;
        AVCodecID codec_id = AV_CODEC_ID_H264;
        std::string resolution = getStrFromJson(json, "data", "resolve", "");
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
        std::string stream_type = getStrFromJson(json, "data", "format", "");
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
        std::string fps = getStrFromJson(json, "data", "fps", "");
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
}

int main(int argc, char** argv){
    int flag = 0;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("debug,d", "enable debug mode")
        ("video,v", po::value<std::string>(), "set video path")
        ("mjpeg,m", "enable MJPEG mode")
        ("jpeg,j", po::value<std::string>(), "set JPEG path")
        ("ram_dcim_url,b", po::value<std::string>(), "set RAM DCIM URL")
        ("rtsp,s", po::value<std::string>(), "set RTSP channel name");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("debug")) {
        flag |= FLAG_DEBUG;
    }

    if (vm.count("video")) {
        flag |= FLAG_VIDEO;
        video_path = vm["video"].as<std::string>();
    }

    if (vm.count("mjpeg")) {
        flag |= FLAG_MJPEG;
    }

    if (vm.count("jpeg")) {
        flag |= FLAG_JPEG;
        jpeg_path = vm["jpeg"].as<std::string>();
    }

    if (vm.count("ram_dcim_url")) {
        ram_dcim_url = vm["ram_dcim_url"].as<std::string>();
    }

    if (vm.count("rtsp")) {
        flag |= FLAG_RTSP;
        rtsp_channel_name = vm["rtsp"].as<std::string>();
    }

    init_jpeg_feedback_gpio();

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
        live555_server = new Live555Server(std::string(rtsp_channel_name));
        stream_receiver->addConsumer(E_CPU_IF_COMMAND_STREAM_VIDEO, 0, live555_server);
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
            handle_sub_msg(received_msg);
        }
        communicator->receiveCmd(handle_cmd);
    }


    stream_receiver->stop();
    std::cout << "receive stoped" << std::endl;
    return 0;
}
