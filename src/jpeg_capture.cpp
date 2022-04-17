#include "jpeg_capture.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <vector>

static bool save_jpeg(const char *fn, void* addr, unsigned long size) {
    std::cout << "file name = " << fn << std::endl;
    std::ofstream os;
    os.open(fn, std::ofstream::binary);
    os.write(static_cast<const char*>(addr), size);
    os.close();
    return os.good();
}



JpegCapture::JpegCapture(int stream_id, std::string path) : FrameConsumer(stream_id){
    filePath = new std::string(path);
    std::cout << "JpegCapture::JpegCapture filePath = " << *filePath << std::endl;

}

JpegCapture::~JpegCapture() {
    
}

void JpegCapture::onFrameReceivedCallback(void* address, std::uint64_t size) {
    std::time_t t = std::time(0);
    std::tm * now = std::localtime(&t); 
    char time_str[100];
    std::strftime(time_str, 100, "%y%m%d%H%M%S", now);
    std::string name = *this->filePath + std::string("/") + std::string(JPEG_PREFIX_STRING) + 
                                std::string(time_str) + std::string(JPEG_SUFFIX_STRING);
    if(save_jpeg(name.c_str(), address, size)){
        if(onSavedCallback != NULL){
            onSavedCallback(name, onSavedCallbackData);
        }
    }
}

int JpegCapture::stop() {
    
}

int JpegCapture::start() {

}

void JpegCapture::setSavedCallback(SavedCallback cb, void* data){
    this->onSavedCallback = cb;
    this->onSavedCallbackData = data;
}