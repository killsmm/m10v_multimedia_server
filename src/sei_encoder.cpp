#include "sei_encoder.h"
#include <cstring>
#include <vector>
#include <iostream>

float *SeiEncoder::longitude = nullptr;
float *SeiEncoder::latitude = nullptr;
float *SeiEncoder::altitude = nullptr;
float *SeiEncoder::roll = nullptr;
float *SeiEncoder::pitch = nullptr;
float *SeiEncoder::yaw = nullptr;
uint8_t *SeiEncoder::encodedData = nullptr;
uint8_t *SeiEncoder::addShellEncodedData = nullptr;

void SeiEncoder::init() {
    if (SeiEncoder::encodedData == nullptr){
        SeiEncoder::encodedData = new uint8_t[SEI_BUF_LENGTH];
    }
    if (SeiEncoder::addShellEncodedData == nullptr){
        SeiEncoder::addShellEncodedData = new uint8_t[128];
    }
    SeiEncoder::latitude = reinterpret_cast<float*>(SeiEncoder::encodedData + 23);
    SeiEncoder::longitude = reinterpret_cast<float*>(SeiEncoder::encodedData + 23 + 4);
    SeiEncoder::altitude = reinterpret_cast<float*>(SeiEncoder::encodedData + 23 + 8);
    SeiEncoder::roll = reinterpret_cast<float*>(SeiEncoder::encodedData + 23 + 12);
    SeiEncoder::pitch = reinterpret_cast<float*>(SeiEncoder::encodedData + 23 + 16);
    SeiEncoder::yaw = reinterpret_cast<float*>(SeiEncoder::encodedData + 23 + 20);
    uint8_t sei_head[23] = {0x00, 0x00, 0x00, 0x01, 0x06, 0x05, 0x28, 
                            0x13, 0x9F, 0xB1, 0xA9, 0x44, 0x6A, 0x4D, 0xEC, 
                            0x8C, 0xBF, 0x65, 0xB1, 0xE1, 0x2D, 0x2C, 0xFD};

    memcpy(SeiEncoder::encodedData, sei_head, 23);
    memset(SeiEncoder::encodedData + 23, 0x00, SEI_BUF_LENGTH - 23);
    *(SeiEncoder::encodedData + 23 + 24) = 0x80;
}

void SeiEncoder::deinit(){
    delete SeiEncoder::encodedData;
}

uint8_t *SeiEncoder::getEncodedSei(int *length) {
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
    std::cout << "sei length = " << *length << std::endl;
    return SeiEncoder::encodedData;
}

void SeiEncoder::setLocation(float latitude, float longitude, float altitude) {
    *SeiEncoder::latitude = latitude;
    *SeiEncoder::longitude = longitude;
    *SeiEncoder::altitude = altitude;
}

void SeiEncoder::setAngles(float roll, float pitch, float yaw) {
    *SeiEncoder::roll = roll;
    *SeiEncoder::pitch = pitch;
    *SeiEncoder::yaw = yaw;
}