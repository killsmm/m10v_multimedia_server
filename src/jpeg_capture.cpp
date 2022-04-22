#include "jpeg_capture.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <vector>



JpegCapture::JpegCapture(std::string path){
    filePath = path;
    std::cout << "JpegCapture::JpegCapture filePath = " << filePath << std::endl;

}

JpegCapture::~JpegCapture() {
    
}

void JpegCapture::onFrameReceivedCallback(void* address, std::uint64_t size) {
    std::time_t t = std::time(0);
    std::tm * now = std::localtime(&t); 
    char time_str[100] = {0};
    std::strftime(time_str, 100, "%y%m%d%H%M%S", now);
    std::string name = this->filePath + std::string("/") + std::string(JPEG_PREFIX_STRING) + 
                                std::string(time_str) + std::string(JPEG_SUFFIX_STRING);
    if(FrameConsumer::save_frame_to_file(name.c_str(), address, size)){
        if(onSavedCallback != NULL){
            onSavedCallback(name, onSavedCallbackData);
        }
    }
}

void JpegCapture::setSavedCallback(SavedCallback cb, void* data){
    this->onSavedCallback = cb;
    this->onSavedCallbackData = data;
}

void JpegCapture::setPath(std::string path){
    this->filePath = path;
}
