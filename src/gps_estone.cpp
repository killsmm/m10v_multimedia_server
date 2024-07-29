#include "json-c/json.h"
#include "utils.h"
#include "gps_estone.h"
#include <cmath>
#include <syslog.h>

using namespace std;
using namespace utils;

static int string_to_float(const char* str, float *result){
    char *tmp;
    *result = std::strtof(str, &tmp);
    if(tmp == str){
        return -1;
    }
    return 0;
}

static int string_to_int(const char* str, int32_t *result){
    char *tmp;
    *result = std::strtol(str, &tmp, 10);
    if(tmp == str){
        return -1;
    }
    return 0;
}

static int validate_gps(int32_t latitude, int32_t longitude, int32_t altitude, float roll, float pitch, float yaw){
    //TODO
    return 0;
}

GPSEstone::GPSEstone() {
    this->gps_data_buf = new boost::circular_buffer<struct gps_data_t>(GPS_BUF_LENGTH);
    this->timing_offset = 0;
}

GPSEstone* GPSEstone::getInstance() {
    static GPSEstone *instance = nullptr;
    if(instance == nullptr){
        instance = new GPSEstone();
    }
    return instance;
}

GPSEstone::~GPSEstone() {

}

void GPSEstone::setTimingOffset(int offset){
    this->timing_offset = offset;
}

int GPSEstone::getTimingOffset(){
    return this->timing_offset;
}

int GPSEstone::getGPSData(gps_data_t *data, uint64_t time_stamp){
    
    if(gps_data_buf->empty()){
        syslog(LOG_WARNING, "The gps_data_buf is empty and return -1");
        return -1;
    }
    
    lock_guard<mutex> lock(this->dataMutex);
    if(time_stamp == 0){
        *data = gps_data_buf->back();
    }else{
        for(auto it = gps_data_buf->begin(); it != gps_data_buf->end(); it++){
            if(time_stamp < it->time_stamp && it != gps_data_buf->begin()){
                syslog(LOG_INFO, "The time_stamp(%lld) is between %lld and %lld", time_stamp, (it - 1)->time_stamp, it->time_stamp);
                *data = *(it - 1);
                break;
            }else if(time_stamp == it->time_stamp){
                syslog(LOG_INFO, "The time_stamp(%lld) is equal to %lld", time_stamp, it->time_stamp);
                *data = *it;
                break;
            }else if (it == gps_data_buf->end() - 1){
                syslog(LOG_WARNING, "The time_stamp(%lld) is too large and return the last data(%lld)", time_stamp, it->time_stamp);
                *data = *it;
                /* 判断GPS断开连接，gps清零 */
                uint64_t frame_time_stamp = 0;
                auto currentTime = std::chrono::high_resolution_clock::now();
                std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch());
                frame_time_stamp = ms.count();
                if(frame_time_stamp > this->gps_rcv_tmout)
                {
                    if((frame_time_stamp - this->gps_rcv_tmout) > 2*1000)
                    {
                        data->altitude = 0xffffffff;
                        data->latitude = 0xffffffff;
                        data->longitude = 0xffffffff;
                        data->pitch = 0xffffffff;
                        data->roll = 0xffffffff;
                        data->yaw = 0xffffffff;
                    }
                }
                /**************************/
                break;
            }
        }
    }
    return 0;
}


int GPSEstone::handleData(json_object *json) {
        int32_t latitude = 0;
        int32_t longitude = 0;
        int32_t altitude = 0;
        float roll = 0;
        float pitch = 0;
        float yaw = 0;
        
        if(string_to_int(getStrFromJson(json, "data", "location", "latitude").data(), &latitude)){
            return -1;
        }
        if(string_to_int(getStrFromJson(json, "data", "location", "longitude").data(), &longitude)){
            return -1;
        }
        if(string_to_int(getStrFromJson(json, "data", "location", "altitude").data(), &altitude)){
            return -1;
        }
        if(string_to_float(getStrFromJson(json, "data", "angles", "roll").data(), &roll)){
            return -1;
        }
        if(string_to_float(getStrFromJson(json, "data", "angles", "pitch").data(), &pitch)){
            return -1;
        }
        if(string_to_float(getStrFromJson(json, "data", "angles", "yaw").data(), &yaw)){
            return -1;
        }

        if(validate_gps(latitude, longitude, altitude, roll, pitch, yaw) != 0){
            return -1;
        }

        uint64_t time_stamp = stoull(getStrFromJson(json, "data", "time_stamp", ""));
        lock_guard<mutex> lock(this->dataMutex);
        if (!this->gps_data_buf->empty() &&  time_stamp - this->gps_data_buf->back().time_stamp > 1000){
            syslog(LOG_ERR, "The time stamp is not continuous, got last time stamp %lld ms ago", time_stamp - this->gps_data_buf->back().time_stamp);
        }
        /* 获取当前时间戳，用于判断GPS数据超时 */
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime.time_since_epoch());
        this->gps_rcv_tmout = ms.count();
        /*********************************** */
        this->gps_data_buf->push_back({latitude, longitude, altitude, roll, pitch, yaw, time_stamp});
        return 0;
}