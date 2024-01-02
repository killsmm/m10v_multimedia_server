#include "json-c/json.h"
#include "utils.h"
#include "gps_estone.h"
#include <cmath>

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
    //if time_stamp == 0, then get the latest data
    
    if(time_stamp == 0){
        if(gps_data_buf->empty()){
            return -1;
        }
        *data = gps_data_buf->back();
        return 0;
    }else{
        //if time_stamp != 0, then get the data with the nearest time_stamp
        if(gps_data_buf->empty()){
            return -1;
        }
        int i = 0;
        for(; i < gps_data_buf->size(); i++){
            if(gps_data_buf->at(i).time_stamp > time_stamp){
                break;
            }
        }
        if(i == 0){
            *data = gps_data_buf->front();
            return 0;
        }else if(i == gps_data_buf->size()){
            *data = gps_data_buf->back();
            return 0;
        }else{
            if(time_stamp - gps_data_buf->at(i-1).time_stamp < gps_data_buf->at(i).time_stamp - time_stamp){
                *data = gps_data_buf->at(i-1);
                return 0;
            }else{
                *data = gps_data_buf->at(i);
                return 0;
            }
        }
    }
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

        uint64_t time_stamp = std::stoull(getStrFromJson(json, "data", "time_stamp", ""));
        this->gps_data_buf->push_back({latitude, longitude, altitude, roll, pitch, yaw, time_stamp});
        return 0;
}