#include "jpeg_capture.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <algorithm>
#include <vector>
#include <chrono>
#include <libexif/exif-format.h>
#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>
#include <libexif/exif-loader.h>

extern "C"{
    #include "jpeg-data.h"
}

JpegCapture::JpegCapture(std::string path){
    filePath = path;
    std::cout << "JpegCapture::JpegCapture filePath = " << filePath << std::endl;

}

JpegCapture::~JpegCapture() {
    
}

void JpegCapture::onFrameReceivedCallback(void* address, std::uint64_t size, void *extra_data) {
    std::time_t t = std::time(0);
    std::tm * now = std::localtime(&t);
    uint64_t now_us = std::chrono::system_clock::now().time_since_epoch().count();
    char time_str[100] = {0};
    std::strftime(time_str, 100, "%y%m%d%H%M%S", now);
    std::string name = this->filePath + std::string("/") + std::string(JPEG_PREFIX_STRING) + 
                                std::string(time_str) + std::to_string(now_us) +std::string(JPEG_SUFFIX_STRING);

    
    T_BF_DCF_IF_EXIF_INFO *info = static_cast<T_BF_DCF_IF_EXIF_INFO*>(extra_data);
    if(saveJpegWithExif(address, size, *info, name.c_str()) == 0){
    // if(FrameConsumer::save_frame_to_file(name.c_str(), address, size)){
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

bool JpegCapture::saveJpegWithExif(void *address, std::uint64_t size, T_BF_DCF_IF_EXIF_INFO info, const char *path)
{
    JPEGData *jpegData = jpeg_data_new_from_data((uint8_t *)address, size);
    
	ExifData* exif = exif_data_new();
	if (!exif) {
        printf("create exif failed\n");
        return false;
    }

    exif_data_dump(exif);
    ExifByteOrder byteOrder = exif_data_get_byte_order(exif);
    ExifContent *content = exif_content_new();

    ExifEntry * exifEntryISO = exif_entry_new();
    exifEntryISO = exif_entry_new();
    exifEntryISO->tag = EXIF_TAG_ISO_SPEED_RATINGS;
    exifEntryISO->format = EXIF_FORMAT_SHORT;
    exifEntryISO->size = 1;
    exifEntryISO->components = 1;
    exifEntryISO->data = (uint8_t *)(&(info.iso_value));
    exif_content_add_entry(content, exifEntryISO);

    ExifEntry * exifEntryShuter = exif_entry_new();
    exifEntryShuter = exif_entry_new();
    exifEntryShuter->tag = EXIF_TAG_ISO_SPEED_RATINGS;
    exifEntryShuter->format = EXIF_FORMAT_FLOAT;
    exifEntryShuter->size = 1;
    exifEntryShuter->components = 1;
    float shutter_speed = ((float)info.shutter_speed.nume) / info.shutter_speed.denomi;
    exifEntryShuter->data = (uint8_t *)(&shutter_speed);
    exif_content_add_entry(content, exifEntryShuter);

    exif->ifd[EXIF_IFD_EXIF] = content;
    exif_data_dump(exif);

    jpeg_data_set_exif_data(jpegData, exif);

    int ret = jpeg_data_save_file(jpegData, path);
    exif_data_free(exif);
    jpeg_data_free(jpegData);
    return ret == 0;
}
