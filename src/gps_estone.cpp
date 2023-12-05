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

static int validate_gps(float latitude, float longitude, float altitude, float roll, float pitch, float yaw){
    if (latitude < -90.0 || latitude > 90.0 || (fabs(latitude) < 0.0001f && latitude != 0.0f)){
        return -1;
    }
    if (longitude < -180.0 || longitude > 180.0 || (fabs(longitude) < 0.0001f && longitude != 0.0f)){
        return -1;
    }
    if (altitude < -1000.0 || altitude > 10000.0 || (fabs(altitude) < 0.0001f && altitude != 0.0f)){
        return -1;
    }
    if (roll < -180.0 || roll > 180.0 || (fabs(roll) < 0.0001f && roll != 0.0f)){
        return -1;
    }
    if (pitch < -180.0 || pitch > 180.0 || (fabs(pitch) < 0.0001f && pitch != 0.0f)){
        return -1;
    }
    if (yaw < -180.0 || yaw > 180.0 || (fabs(yaw) < 0.0001f && yaw != 0.0f)){
        return -1;
    }
    return 0;
}

GPSEstone::GPSEstone() {
    gps_data_buf = new boost::circular_buffer<struct gps_data_t>(GPS_BUF_LENGTH);
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

int GPSEstone::getGPSData(gps_data_t *data, long time_stamp){
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
        float latitude = 0;
        float longitude = 0;
        float altitude = 0;
        float roll = 0;
        float pitch = 0;
        float yaw = 0;
        
        if(string_to_float(getStrFromJson(json, "data", "location", "latitude").data(), &latitude)){
            return -1;
        }
        if(string_to_float(getStrFromJson(json, "data", "location", "longitude").data(), &longitude)){
            return -1;
        }
        if(string_to_float(getStrFromJson(json, "data", "location", "altitude").data(), &altitude)){
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

        long time_stamp = std::stol(getStrFromJson(json, "data", "time_stamp", ""));
        this->gps_data_buf->push_back({latitude, longitude, altitude, roll, pitch, yaw, time_stamp});
        return 0;
}