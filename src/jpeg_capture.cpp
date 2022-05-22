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

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

JpegCapture::JpegCapture(std::string path){
    filePath = path;
    std::cout << "JpegCapture::JpegCapture filePath = " << filePath << std::endl;
    this->prefix = std::string(JPEG_DEFAULT_PREFIX_STRING);
}

JpegCapture::~JpegCapture() {
    
}

void JpegCapture::onFrameReceivedCallback(void* address, std::uint64_t size, void *extra_data) {
    if(pthread_mutex_trylock(&lock) != 0){
        std::cout << "jpeg try lock failed" << std::endl;
        return;
    }
    std::time_t t = std::time(0);
    std::tm * now = std::localtime(&t);
    uint64_t now_us = std::chrono::system_clock::now().time_since_epoch().count();
    char time_str[100] = {0};
    std::strftime(time_str, 100, "%y%m%d%H%M%S", now);
    std::string name = this->filePath + std::string("/") + this->prefix + 
                                std::string(time_str) + std::to_string(now_us) + std::string(JPEG_SUFFIX_STRING);

    // std::cout << (int) extra_data << std::endl;
    T_BF_DCF_IF_EXIF_INFO *info = (T_BF_DCF_IF_EXIF_INFO*)(extra_data);

    if(saveJpegWithExif(address, size, *info, name.c_str()) == 0){
    // if(FrameConsumer::save_frame_to_file(name.c_str(), address, size)){
        if(onSavedCallback != NULL){
            onSavedCallback(name, onSavedCallbackData);
        }
    }
    pthread_mutex_unlock(&lock);
}

void JpegCapture::setSavedCallback(SavedCallback cb, void* data){
    this->onSavedCallback = cb;
    this->onSavedCallbackData = data;
}

void JpegCapture::setPath(std::string path){
    this->filePath = path;
}

void JpegCapture::setPrefix(std::string prefix){
    this->prefix = prefix;
}


static ExifEntry * new_exif_entry(ExifTag tag, ExifFormat fmt, uint8_t *data, uint32_t size){
    ExifEntry * entry = exif_entry_new();
    entry->tag = tag;
    entry->format = fmt;
    entry->components = 1;
    entry->size = size;
    entry->data = data;
    return entry;
}

bool JpegCapture::saveJpegWithExif(void *address, std::uint64_t size, T_BF_DCF_IF_EXIF_INFO info, const char *path)
{
    // printf("exif size = %d\n", sizeof(info));
    // std::cout << "iso = " << info.iso_value << std::endl;
    // std::cout << "exp_time = " << info.exposure_time.nume << "/" << info.exposure_time.denomi << std::endl;

    JPEGData *jpegData = jpeg_data_new_from_data((uint8_t *)address, size);
    
	ExifData* exif = exif_data_new();
	if (!exif) {
        printf("create exif failed\n");
        return false;
    }

    
    ExifByteOrder byteOrder = exif_data_get_byte_order(exif);
    ExifContent *content = exif_content_new();

    /* iso */
    uint32_t iso_data_size = exif_format_get_size(EXIF_FORMAT_SHORT);
    uint8_t iso_data[iso_data_size];
    exif_set_short(iso_data, byteOrder, info.iso_value);
    ExifEntry * exifEntryISO = new_exif_entry(EXIF_TAG_ISO_SPEED_RATINGS, EXIF_FORMAT_SHORT, 
                                                iso_data, iso_data_size);
    exif_content_add_entry(content, exifEntryISO);

    /* exposure time */
    uint32_t shutter_speed_data_size = exif_format_get_size(EXIF_FORMAT_RATIONAL);
    uint8_t shutter_speed_data[shutter_speed_data_size];
    exif_set_rational(shutter_speed_data, byteOrder, ExifRational{
                                                .numerator = info.exposure_time.nume,
                                                .denominator = info.exposure_time.denomi
                                                });
    ExifEntry * exifEntryShutter = new_exif_entry(EXIF_TAG_EXPOSURE_TIME, EXIF_FORMAT_RATIONAL, 
                                                    shutter_speed_data, shutter_speed_data_size);    
    exif_content_add_entry(content, exifEntryShutter);

    exif->ifd[EXIF_IFD_EXIF] = content;
    // exif_data_dump(exif);

    jpeg_data_set_exif_data(jpegData, exif);

    int ret = jpeg_data_save_file(jpegData, path);
    exif_data_free(exif);
    jpeg_data_free(jpegData);
    return ret == 0;
}
