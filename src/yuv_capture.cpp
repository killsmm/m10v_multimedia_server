#include "yuv_capture.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <vector>




YuvCapture::YuvCapture(std::string path){
    filePath = path;
    std::cout << "YuvCapture::RawImgCapture filePath = " << filePath << std::endl;

}

YuvCapture::~YuvCapture() {
    
}

void YuvCapture::onFrameReceivedCallback(void* address, std::uint64_t size,  void *extra_data) {
    std::time_t t = std::time(0);
    std::tm * now = std::localtime(&t); 
    char time_str[100] = {0};
    std::strftime(time_str, 100, "%y%m%d%H%M%S", now);
    std::string name = this->filePath + std::string("/") + std::string(YUV_PREFIX_STRING) + 
                                std::string(time_str) + std::string(YUV_SUFFIX_STRING);
    if(FrameConsumer::save_frame_to_file(name.c_str(), address, size)){
        if(onSavedCallback != NULL){
            onSavedCallback(name, onSavedCallbackData);
        }
    }
}

void YuvCapture::setSavedCallback(SavedCallback cb, void* data){
    this->onSavedCallback = cb;
    this->onSavedCallbackData = data;
}

void YuvCapture::setPath(std::string path){
    this->filePath = path;
}
