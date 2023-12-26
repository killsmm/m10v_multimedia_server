#include "sei_encoder.h"
#include <cstring>
#include <vector>
#include <iostream>
#include <pthread.h>
#include "gps_estone.h"

const uint8_t SEI_HEAD[23] = {0x00, 0x00, 0x00, 0x01, 0x06, 0x05, 0x28, 
                        0x13, 0x9F, 0xB1, 0xA9, 0x44, 0x6A, 0x4D, 0xEC, 
                        0x8C, 0xBF, 0x65, 0xB1, 0xE1, 0x2D, 0x2C, 0xFD};


void SeiEncoder::encode(struct gps_data_t data, uint8_t *encoded_data, int *length){
    int zero_counter = 0;
    std::vector<uint8_t> a;

    uint8_t tmp[24];

    memcpy(tmp, &data.longitude , 4);
    memcpy(tmp + 4, &data.latitude , 4);
    memcpy(tmp + 8, &data.altitude , 4);
    memcpy(tmp + 12, &data.pitch , 4);
    memcpy(tmp + 16, &data.yaw , 4);
    memcpy(tmp + 20, &data.roll , 4);

    for(int i = 0; i < 24; i++){
        //if tmp[i-1] and tmp[i] are both 0x00, then add 0x03
        a.push_back(tmp[i]);
        if(tmp[i] == 0x00){
            zero_counter++;
        }else{
            zero_counter = 0;
        }
        if(zero_counter == 2){
            a.push_back(0x03);
            zero_counter = 0;
        }
    }

    memcpy(encoded_data, SEI_HEAD, 23);
    memcpy(encoded_data + 23, a.data(), a.size());
    *length = a.size() + 23;

}

#if 0

volatile float *SeiEncoder::longitude = nullptr;
volatile float *SeiEncoder::latitude = nullptr;
volatile float *SeiEncoder::altitude = nullptr;
volatile float *SeiEncoder::roll = nullptr;
volatile float *SeiEncoder::pitch = nullptr;
volatile float *SeiEncoder::yaw = nullptr;
uint8_t *SeiEncoder::encodedData = nullptr;
uint8_t *SeiEncoder::addShellEncodedData = nullptr;
volatile long SeiEncoder::time_stamp = 0;
static pthread_mutex_t mutex;


void SeiEncoder::init() {
    if (SeiEncoder::encodedData == nullptr){
        SeiEncoder::encodedData = new uint8_t[SEI_BUF_LENGTH];
    }
    if (SeiEncoder::addShellEncodedData == nullptr){
        SeiEncoder::addShellEncodedData = new uint8_t[128];
    }
    SeiEncoder::longitude = reinterpret_cast<volatile float*>(SeiEncoder::encodedData + 23);
    SeiEncoder::latitude = reinterpret_cast<volatile float*>(SeiEncoder::encodedData + 23 + 4);
    SeiEncoder::altitude = reinterpret_cast<volatile float*>(SeiEncoder::encodedData + 23 + 8);
    SeiEncoder::pitch = reinterpret_cast<volatile float*>(SeiEncoder::encodedData + 23 + 12);
    SeiEncoder::yaw = reinterpret_cast<volatile float*>(SeiEncoder::encodedData + 23 + 16);
    SeiEncoder::roll = reinterpret_cast<volatile float*>(SeiEncoder::encodedData + 23 + 20);
    uint8_t sei_head[23] = {0x00, 0x00, 0x00, 0x01, 0x06, 0x05, 0x28, 
                            0x13, 0x9F, 0xB1, 0xA9, 0x44, 0x6A, 0x4D, 0xEC, 
                            0x8C, 0xBF, 0x65, 0xB1, 0xE1, 0x2D, 0x2C, 0xFD};

    memcpy(SeiEncoder::encodedData, sei_head, 23);
    memset(SeiEncoder::encodedData + 23, 0x00, SEI_BUF_LENGTH - 23);
    *(SeiEncoder::encodedData + 23 + 24) = 0x80;
    SeiEncoder::time_stamp = 0;
    // setLocation(66.0, 66.0, 66.0);
    // setAngles(66.0, -66.0, 66.0);
}

void SeiEncoder::deinit(){
    delete SeiEncoder::encodedData;
}

void SeiEncoder::getEncodedSei(int *length, uint8_t *data) {
    int zero_counter = 0;
    std::vector<uint8_t> a;
    for(int i = 23; i < (23 + 24); i++){
        a.push_back(SeiEncoder::encodedData[i]);
        if(SeiEncoder::encodedData[i] == 0x00){
            zero_counter++;
        }else{
            zero_counter = 0;
        }
        if(zero_counter == 2){
            a.push_back(0x03);
            zero_counter = 0;
        }
        
    }
    memcpy(SeiEncoder::addShellEncodedData, SeiEncoder::encodedData, 23);
    memcpy(SeiEncoder::addShellEncodedData + 23, a.data(), a.size());
    *length = a.size() + 23;
    memcpy(data, SeiEncoder::addShellEncodedData, *length);
}

void SeiEncoder::setLocation(float latitude, float longitude, float altitude) {
    //add a pthread mutex here
    memcpy(SeiEncoder::encodedData + 23, &longitude, 4); 
    memcpy(SeiEncoder::encodedData + 23 + 4, &latitude, 4); 
    memcpy(SeiEncoder::encodedData + 23 + 8, &altitude, 4); 
}

void SeiEncoder::setAngles(float roll, float pitch, float yaw) {
    memcpy(SeiEncoder::encodedData + 23 + 12, &pitch, 4); 
    memcpy(SeiEncoder::encodedData + 23 + 16, &yaw, 4); 
    memcpy(SeiEncoder::encodedData + 23 + 20, &roll, 4); 
}

float SeiEncoder::getLatitude(){
    float tmp;
    memcpy(SeiEncoder::encodedData + 23, reinterpret_cast<uint8_t*>(&tmp), 4);
    return tmp;
}

timeb SeiEncoder::getFrameTime(){
    timeb t;
    ftime(&t);
    //t.time is in seconds
    t.time = SeiEncoder::time_stamp;
    return t;
}
#endif