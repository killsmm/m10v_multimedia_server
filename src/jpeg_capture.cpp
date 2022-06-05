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
#include <cstring>
#include "device_status.h"
#include "sei_encoder.h"

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


static void new_exif_entry(ExifContent *content, ExifTag tag, ExifFormat fmt, uint8_t *data, uint32_t component_number){
    ExifEntry * entry = exif_entry_new();
    entry->tag = tag;
    entry->format = fmt;
    entry->components = component_number;
    entry->size = exif_format_get_size(fmt) * component_number;
    entry->data = data;
    exif_content_add_entry(content, entry);
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
    new_exif_entry(content, EXIF_TAG_ISO_SPEED_RATINGS, EXIF_FORMAT_SHORT, 
                                                iso_data, 1);

    /* exposure time */
    uint32_t shutter_speed_data_size = exif_format_get_size(EXIF_FORMAT_RATIONAL);
    uint8_t shutter_speed_data[shutter_speed_data_size];
    exif_set_rational(shutter_speed_data, byteOrder, ExifRational{
                                                .numerator = info.exposure_time.nume,
                                                .denominator = info.exposure_time.denomi
                                                });
    new_exif_entry(content, EXIF_TAG_EXPOSURE_TIME, EXIF_FORMAT_RATIONAL, 
                                                    shutter_speed_data, 1);
    
    /* metring mode */
    uint8_t metering_mode[2];
    exif_set_short(metering_mode, byteOrder, info.metering);
    new_exif_entry(content, EXIF_TAG_METERING_MODE, EXIF_FORMAT_SHORT, metering_mode, 1);

    /* camera make */
    char make[] = "Feituo";
    new_exif_entry(content, EXIF_TAG_MAKE, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(make), strlen(make));

    /* camera model */
    char model[] = "S571";
    new_exif_entry(content, EXIF_TAG_MODEL, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(model), strlen(model));

    char software_version[] = "v0.1.2";
    new_exif_entry(content, EXIF_TAG_SOFTWARE, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(software_version), strlen(software_version));

    /* shutter number */
    struct maker_note {
        uint32_t shutter_count; 
        float cmos_temperature; // degree
        float input_voltage;
        int32_t noise_reduction_strength; // 0 - 4 low to high
        int32_t jpeg_quality_level; // 0 - 4 low to high 0 for 70, 1 for 80, 2 for 90, 3 for 100
        int32_t shutter_mode; // 0 for electrical , 1 for mechnical
    } m_note;

    m_note.shutter_count = DeviceStatus::shutter_count; //from broadcast
    m_note.cmos_temperature = DeviceStatus::cmos_temperature; //from broadcast
    m_note.input_voltage = DeviceStatus::input_voltage; //from broadcast
    m_note.noise_reduction_strength = DeviceStatus::noise_reduction_strength; //from broadcast
    m_note.jpeg_quality_level = DeviceStatus::jpeg_quality_level; //from broadcast
    m_note.shutter_mode = DeviceStatus::shutter_mode;  //from broadcast

    new_exif_entry(content, EXIF_TAG_MAKER_NOTE, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(&m_note), sizeof(m_note));

    exif->ifd[EXIF_IFD_EXIF] = content;

/* GPS  */
    ExifContent *gps_content = exif_content_new();
    ExifRational tmp = {0};

    printf("%f, %f, %f\n", *SeiEncoder::altitude, *SeiEncoder::latitude, *SeiEncoder::longitude);

    uint8_t altitude_data[16];
    tmp.denominator = 1000000;
    tmp.numerator = *SeiEncoder::altitude * 1000000;
    exif_set_rational(altitude_data, byteOrder, tmp);
    new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_ALTITUDE, EXIF_FORMAT_RATIONAL, altitude_data, 1);

    uint8_t longitude_data[16];
    tmp.denominator = 1000000;
    tmp.numerator = *SeiEncoder::longitude * 1000000;
    exif_set_rational(longitude_data, byteOrder, tmp);
    new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_LONGITUDE, EXIF_FORMAT_RATIONAL, longitude_data, 1);

    uint8_t latitude_data[16];
    tmp.denominator = 1000000;
    tmp.numerator = *SeiEncoder::latitude * 1000000;
    exif_set_rational(latitude_data, byteOrder, tmp);
    new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_LATITUDE, EXIF_FORMAT_RATIONAL, latitude_data, 1);

    exif->ifd[EXIF_IFD_GPS] = gps_content;
    // exif_data_dump(exif);
/* GPS END */

    jpeg_data_set_exif_data(jpegData, exif);

    int ret = jpeg_data_save_file(jpegData, path);
    exif_data_free(exif);
    jpeg_data_free(jpegData);
    return ret == 0;
}
