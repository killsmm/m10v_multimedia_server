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
#include <sys/time.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
// #include <arm_neon.h>
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

static void print_operation_time(const char* s){
    static uint64_t last ;
    struct timeval tm;
    gettimeofday(&tm, nullptr);
    uint64_t now = (tm.tv_sec * 1000) + (tm.tv_usec / 1000);
    printf("[OPERATION_TIME]%s : %d\n", s,  now - last);
    last = now;
}


void my_copy(volatile unsigned char *dst, volatile unsigned char *src, int sz)
{
    // if (sz & 63) {
    //     sz = (sz & -64) + 64;
    // }
    // asm volatile (
    //     "NEONCopyPLD:                          \n"
    //     "    VLDM %[src]!,{d0-d7}                 \n"
    //     "    VSTM %[dst]!,{d0-d7}                 \n"
    //     "    SUBS %[sz],%[sz],#0x40                 \n"
    //     "    BGT NEONCopyPLD                  \n"
    //     : [dst]"+r"(dst), [src]"+r"(src), [sz]"+r"(sz) : : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc", "memory");
}

static std::string get_jpeg_full_path_now(std::string parent_path, std::string prefix, std::string suffix){
    struct timeval tv;
    time_t curtime;
    gettimeofday(&tv, NULL);
    curtime = tv.tv_sec;
    char time_str[100] = {0};
    strftime(time_str,100,"%y%m%d%H%M%S",localtime(&curtime));
    std::string full_path = parent_path + std::string("/") + prefix + std::string(time_str) + 
                            std::to_string(tv.tv_usec / 1000) + suffix; 
    return full_path;
}


void JpegCapture::onFrameReceivedCallback(void* address, std::uint64_t size, void *extra_data) {
    if(pthread_mutex_trylock(&lock) != 0){
        std::cout << "jpeg try lock failed" << std::endl;
        return;
    }
    // std::time_t t = std::time(0);
    // std::tm * now = std::localtime(&t);
    
    // uint64_t now_us = std::chrono::system_clock::now().time_since_epoch().count();
    
    // char time_str[100] = {0};
    // std::strftime(time_str, 100, "%y%m%d%H%M%S", now);
    // std::string name = this->filePath + std::string("/") + this->prefix + 
    //                             std::string(time_str) + std::to_string(now_us) + std::string(JPEG_SUFFIX_STRING);

    std::string name = get_jpeg_full_path_now(this->filePath, this->prefix, std::string(JPEG_SUFFIX_STRING));
    std::cout << name << std::endl;
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
    exif_entry_ref(entry);
}

static void dump_exif_info(T_BF_DCF_IF_EXIF_INFO &info){
    printf("----------exif from rtos-------------\n");
    printf("image width = %d, lines = %d\n", info.width, info.lines);
    printf("data: %d-%d-%d %d:%d:%d\n", info.date.ad_year, info.date.month, info.date.day,
                                        info.date.hour, info.date.min, info.date.sec);
    printf("iso = %d, shutter_speed = %d/%d, meter_mode = %d\n", info.iso_value, info.shutter_speed.nume, info.shutter_speed.denomi, info.metering);
    printf("exposure_prog = %d, exposure_time = %d/%d, flash = %d, light_source = %d\n", 
                    info.exposure_prog, info.exposure_time.nume, info.exposure_time.denomi, info.flash, info.light_source);
    printf("----------exif from rtos end--------\n");
}

static float get_ev_brightness(float f_number, float exp_time, float iso){
    return log2(f_number * f_number / exp_time) + log2(iso / 100);
}

typedef struct{
    uint32_t degrees;
    uint32_t minutes;
    uint32_t seconds;
}GPS_DEGREE;

static GPS_DEGREE float_to_degree(float decimal){
    GPS_DEGREE ret = {0};
    ret.degrees = static_cast<uint32_t>(decimal);
    ret.minutes = static_cast<uint32_t>((decimal - ret.degrees) * 60);
    ret.seconds = static_cast<uint32_t>((decimal - ret.minutes) * 60 - ret.minutes);
    return ret;
}

static int save_picture_with_exif(const char* full_path, void *address, uint64_t ori_size, ExifData *exif){
    int ret = 0;
    uint32_t exif_length = 0;
    uint8_t *exif_data = NULL;
    exif_data_save_data(exif, &exif_data, &exif_length);
    
    uint32_t jpeg_app_size = exif_length + 2;
    uint8_t jpeg_app_head[4] = {0xff, 0xe1};
    jpeg_app_head[2] = (jpeg_app_size & 0xff00) >> 8;
    jpeg_app_head[3] = jpeg_app_size & 0xff;
#if 1
    int fd = open(full_path, O_CREAT | O_RDWR);
    if(fd < 0){
        printf("open jpeg original file failed\n");
    }else{
        ret |= write(fd, address, 2);
        ret |= write(fd, jpeg_app_head, 4);
        ret |= write(fd, exif_data, exif_length);
        ret |= write(fd, address + 2, ori_size - 2);
        close(fd);
    }
#elif 0
    FILE *img = fopen(full_path, "w+");
    if(img != NULL){
        fwrite(address, 2, 1, img);
        fwrite(jpeg_app_head, 4, 1, img);
        fwrite(exif_data, exif_length, 1, img);
        fwrite(address + 2, ori_size - 2, 1, img);
        fclose(img);
    }
#else
    int fd = open(full_path, O_CREAT | O_RDWR);
    if(fd < 0){
        printf("open jpeg original file failed\n");
    }else{
        void *map_addr = mmap(NULL, 20 * 1024 * 1024, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (map_addr == MAP_FAILED){
            printf("mmap failed\n");
        }else{
            volatile uint8_t *p = reinterpret_cast<uint8_t *> (map_addr);

            ftruncate(fd, ori_size + 4 + exif_length);
            // my_copy(p, reinterpret_cast<uint8_t *>(address), 2);
            // my_copy(p + 2, jpeg_app_head, 4);
            // my_copy(p + 6, exif_data, exif_length);
            // my_copy(p + 6 + exif_length, reinterpret_cast<uint8_t *>(address) + 2, ori_size - 2);
            memcpy(map_addr, reinterpret_cast<uint8_t *>(address), 2);
            memcpy(map_addr + 2, jpeg_app_head, 4);
            memcpy(map_addr + 6, exif_data, exif_length);
            memcpy(map_addr + 6 + exif_length, reinterpret_cast<uint8_t *>(address) + 2, ori_size - 2);
            munmap(map_addr, 20 * 1024 * 1024);
        }
        close(fd);
    }
#endif
    delete exif_data;
    return ret;
}

bool JpegCapture::saveJpegWithExif(void *address, std::uint64_t size, T_BF_DCF_IF_EXIF_INFO info, const char *path)
{
    // printf("exif size = %d\n", sizeof(info));
    // std::cout << "iso = " << info.iso_value << std::endl;
    // std::cout << "exp_time = " << info.exposure_time.nume << "/" << info.exposure_time.denomi << std::endl;

    print_operation_time("first");

    // dump_exif_info(info);


    
    // ExifData *exif = get_exif_from_rtos_info(info);

	ExifData* exif = exif_data_new();
    exif_data_set_byte_order(exif, EXIF_BYTE_ORDER_INTEL);
	if (!exif) {
        printf("create exif failed\n");
        return false;
    }

    
    ExifByteOrder byteOrder = exif_data_get_byte_order(exif);

    ////////////* IFD 0 *////////////////

    ExifContent *ifd_0 = exif_content_new();

    char artist[] = "Fiytoun Icont";
    new_exif_entry(ifd_0, EXIF_TAG_ARTIST, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(artist), 14);

    /* camera make */
    char make[] = "Fiytoun Icont";
    new_exif_entry(ifd_0, EXIF_TAG_MAKE, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(make), 14);

    /* camera model */
    char model[] = "FT1-2616";
    new_exif_entry(ifd_0, EXIF_TAG_MODEL, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(model), strlen(model) + 1);

    char software_version[] = "v0.1.2";
    new_exif_entry(ifd_0, EXIF_TAG_SOFTWARE, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(software_version), strlen(software_version) + 1);

    uint8_t resolution_unit[EXIF_FORMAT_SHORT];
    exif_set_short(resolution_unit, byteOrder, ExifShort(2)); //inches
    new_exif_entry(ifd_0, EXIF_TAG_RESOLUTION_UNIT, EXIF_FORMAT_SHORT, resolution_unit, 1);

    uint8_t x_resolution[exif_format_get_size(EXIF_FORMAT_RATIONAL)];
    exif_set_rational(x_resolution, byteOrder, ExifRational{.numerator = 350, 
                                                            .denominator = 1});
    new_exif_entry(ifd_0, EXIF_TAG_X_RESOLUTION, EXIF_FORMAT_RATIONAL, 
                                                x_resolution, 1);

    uint8_t y_resolution[exif_format_get_size(EXIF_FORMAT_RATIONAL)];
    exif_set_rational(y_resolution, byteOrder, ExifRational{.numerator = 350, 
                                                            .denominator = 1});
    new_exif_entry(ifd_0, EXIF_TAG_Y_RESOLUTION, EXIF_FORMAT_RATIONAL, 
                                                y_resolution, 1);


    exif->ifd[EXIF_IFD_0] = ifd_0;  

    print_operation_time("IFD0_finish");


    ////////////* IFD EXIF *////////////////

    ExifContent *content = exif_content_new();
    /* create date */
    char date_str[100] = {0};
    struct timeval tv;
    gettimeofday(&tv, NULL);
    size_t date_str_length = strftime(date_str,100,"%Y:%m:%d %T:",localtime(&tv.tv_sec));
    strcpy(&date_str[date_str_length], std::to_string(static_cast<int>(tv.tv_usec / 1000)).c_str());
    // printf("date_str_length = %d\n", date_str_length);
    uint32_t total_length = date_str_length + strlen(&date_str[date_str_length]) + 1;
    new_exif_entry(content, EXIF_TAG_DATE_TIME_ORIGINAL, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(date_str), total_length);
    new_exif_entry(content, EXIF_TAG_DATE_TIME, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(date_str), total_length);
    new_exif_entry(content, EXIF_TAG_DATE_TIME_DIGITIZED, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(date_str), total_length);

    
    /* serial number */


    new_exif_entry(content, (ExifTag)0xa431, EXIF_FORMAT_ASCII, 
                                (uint8_t*)(DeviceStatus::serial_number.c_str()), 
                                strlen(DeviceStatus::serial_number.c_str()) + 1);

    new_exif_entry(content, (ExifTag)0xc62f, EXIF_FORMAT_ASCII, 
                                (uint8_t*)(DeviceStatus::serial_number.c_str()), 
                                strlen(DeviceStatus::serial_number.c_str()) + 1);

    std::cout << DeviceStatus::serial_number << std::endl;
    print_operation_time("serial_number");

    /* iso */
    uint32_t iso_data_size = exif_format_get_size(EXIF_FORMAT_SHORT);
    uint8_t iso_data[iso_data_size];
    exif_set_short(iso_data, byteOrder, info.iso_value);
    new_exif_entry(content, EXIF_TAG_ISO_SPEED_RATINGS, EXIF_FORMAT_SHORT, 
                                                iso_data, 1);

    /* exposure time */
    // printf("exposure time = %d/%d\n", info.exposure_time.nume, info.exposure_time.denomi);
    uint32_t shutter_speed_data_size = exif_format_get_size(EXIF_FORMAT_RATIONAL);
    uint8_t shutter_speed_data[shutter_speed_data_size];
    exif_set_rational(shutter_speed_data, byteOrder, ExifRational{
                                                .numerator = info.exposure_time.nume,
                                                .denominator = info.exposure_time.denomi
                                                });
    new_exif_entry(content, EXIF_TAG_EXPOSURE_TIME, EXIF_FORMAT_RATIONAL, 
                                                    shutter_speed_data, 1);

    /* fnumber */
    uint8_t fnumber[exif_format_get_size(EXIF_FORMAT_RATIONAL)]; //APEX value = log2 (fnumber^2)
    exif_set_rational(fnumber, byteOrder, ExifRational{
                                                // .numerator = static_cast<ExifLong> (log2(5.6*5.6) * 1000000),
                                                .numerator = 5600000,
                                                .denominator = 1000000
                                                });
    new_exif_entry(content, EXIF_TAG_FNUMBER, EXIF_FORMAT_RATIONAL, 
                                                    fnumber, 1);

    /* focal length */
    uint8_t focal_length[exif_format_get_size(EXIF_FORMAT_RATIONAL)];
    exif_set_rational(focal_length, byteOrder, ExifRational{
                                                .numerator = 350,
                                                .denominator = 10
                                                });
    new_exif_entry(content, EXIF_TAG_FOCAL_LENGTH, EXIF_FORMAT_RATIONAL, 
                                                    focal_length, 1);

    /* sharpness */
    uint8_t sharpness[exif_format_get_size(EXIF_FORMAT_SHORT)];
    exif_set_short(sharpness, byteOrder, ExifSShort(info.sharpness));
    new_exif_entry(content, EXIF_TAG_SHARPNESS, EXIF_FORMAT_SSHORT, sharpness, 1);

    
    /* metring mode */
    // printf("metering mode = %d\n", info.metering);
    uint8_t metering_mode[exif_format_get_size(EXIF_FORMAT_SHORT)];
    exif_set_short(metering_mode, byteOrder, ExifShort(info.metering));
    new_exif_entry(content, EXIF_TAG_METERING_MODE, EXIF_FORMAT_SHORT, metering_mode, 1);

    /* EV */
    uint8_t ev[exif_format_get_size(EXIF_FORMAT_SRATIONAL)];
    exif_set_srational(ev, byteOrder, ExifSRational{.numerator = static_cast<int>(info.ev * 10), .denominator = 10});
    new_exif_entry(content, EXIF_TAG_EXPOSURE_BIAS_VALUE, EXIF_FORMAT_SRATIONAL, ev, 1);

    /* flash */
    uint8_t flash[exif_format_get_size(EXIF_FORMAT_SHORT)];
    exif_set_short(flash, byteOrder, ExifShort(0));
    new_exif_entry(content, EXIF_TAG_FLASH, EXIF_FORMAT_SHORT, flash, 1);

    /* exif version */
    char version[] = "0230";
    new_exif_entry(content, EXIF_TAG_EXIF_VERSION, EXIF_FORMAT_UNDEFINED, reinterpret_cast<uint8_t *>(version), strlen(version));

    /* brightness */
    uint8_t brightness[exif_format_get_size(EXIF_FORMAT_SRATIONAL)] = {0};
    float ev_brightness = get_ev_brightness(5.6, (static_cast<float>(info.exposure_time.nume)) / info.exposure_time.denomi, info.iso_value);
    // printf("ev_brightness = %f\n", ev_brightness);
    exif_set_srational(brightness, byteOrder, ExifSRational{.numerator = static_cast<ExifSLong>(ev_brightness * 100),
                                                            .denominator = 100});
    new_exif_entry(content, EXIF_TAG_BRIGHTNESS_VALUE, EXIF_FORMAT_SRATIONAL, reinterpret_cast<uint8_t *>(brightness), 1);

    /* maker note */

    char maker_note[512] = {0};
    sprintf(maker_note, "shutter_count=%d\ncmos_temperature=%f\ninput_voltage=%f\njpeg_quality_level=%d\nshutter_mode=%d\n"
                        "internal_code=%s\n",
                                    DeviceStatus::shutter_count,
                                    DeviceStatus::cmos_temperature,
                                    DeviceStatus::input_voltage,
                                    DeviceStatus::jpeg_quality_level,
                                    DeviceStatus::shutter_mode,
                                    DeviceStatus::internal_code.c_str());
    new_exif_entry(content, EXIF_TAG_MAKER_NOTE, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(maker_note), strlen(maker_note) + 1);
    /* shutter number */
    // struct maker_note {
    //     uint32_t shutter_count; 
    //     float cmos_temperature; // degree
    //     float input_voltage;
    //     int32_t noise_reduction_strength; // 0 - 4 low to high
    //     int32_t jpeg_quality_level; // 0 - 4 low to high 0 for 70, 1 for 80, 2 for 90, 3 for 100
    //     int32_t shutter_mode; // 0 for electrical , 1 for mechnical
    // } m_note;

    // m_note.shutter_count = DeviceStatus::shutter_count; //from broadcast
    // m_note.cmos_temperature = DeviceStatus::cmos_temperature; //from broadcast
    // m_note.input_voltage = DeviceStatus::input_voltage; //from broadcast
    // m_note.noise_reduction_strength = DeviceStatus::noise_reduction_strength; //from broadcast
    // m_note.jpeg_quality_level = DeviceStatus::jpeg_quality_level; //from broadcast
    // m_note.shutter_mode = DeviceStatus::shutter_mode;  //from broadcast

    // new_exif_entry(content, EXIF_TAG_MAKER_NOTE, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(&m_note), sizeof(m_note));

    exif->ifd[EXIF_IFD_EXIF] = content;

    print_operation_time("ifd_exif");


/* GPS  */
    ExifContent *gps_content = exif_content_new();
    ExifRational tmp = {0};

    printf("%f, %f, %f\n", *SeiEncoder::altitude, *SeiEncoder::latitude, *SeiEncoder::longitude);

    uint8_t altitude_data[exif_format_get_size(EXIF_FORMAT_RATIONAL)] = {0};
    tmp.denominator = 1000;
    tmp.numerator = *SeiEncoder::altitude * 1000 ? *SeiEncoder::altitude * 1000 : 121333 ;
    exif_set_rational(altitude_data, byteOrder, tmp);
    new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_ALTITUDE, EXIF_FORMAT_RATIONAL, altitude_data, 1);

    uint8_t longitude_data[exif_format_get_size(EXIF_FORMAT_RATIONAL) * 3] = {0};
    GPS_DEGREE longi_degree = float_to_degree(*SeiEncoder::longitude ? *SeiEncoder::longitude : 123.123321);
    exif_set_rational(longitude_data, byteOrder, ExifRational{.numerator = longi_degree.degrees, .denominator = 1});
    exif_set_rational(longitude_data + exif_format_get_size(EXIF_FORMAT_RATIONAL) * 1, byteOrder, ExifRational{.numerator = longi_degree.minutes, .denominator = 1});
    exif_set_rational(longitude_data + exif_format_get_size(EXIF_FORMAT_RATIONAL) * 2, byteOrder, ExifRational{.numerator = longi_degree.seconds, .denominator = 1});
    new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_LONGITUDE, EXIF_FORMAT_RATIONAL, longitude_data, 3);

    uint8_t latitude_data[exif_format_get_size(EXIF_FORMAT_RATIONAL) * 3] = {0};
    GPS_DEGREE lati_degree = float_to_degree(*SeiEncoder::latitude ? *SeiEncoder::latitude : 56.123321);    
    exif_set_rational(latitude_data, byteOrder, ExifRational{.numerator = lati_degree.degrees, .denominator = 1});
    exif_set_rational(latitude_data + exif_format_get_size(EXIF_FORMAT_RATIONAL) * 1, byteOrder, ExifRational{.numerator = lati_degree.minutes, .denominator = 1});
    exif_set_rational(latitude_data + exif_format_get_size(EXIF_FORMAT_RATIONAL) * 2, byteOrder, ExifRational{.numerator = lati_degree.seconds, .denominator = 1});
    new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_LATITUDE, EXIF_FORMAT_RATIONAL, latitude_data, 3);

    char latitude_ref[] = "N";
    new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_LATITUDE_REF, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(latitude_ref), strlen(latitude_ref) + 1);

    char longitude_ref[] = "W";
    new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_LONGITUDE_REF, EXIF_FORMAT_ASCII, reinterpret_cast<uint8_t *>(longitude_ref), strlen(longitude_ref) + 1);

    uint8_t altitude_ref[] = {0};
    new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_ALTITUDE_REF, EXIF_FORMAT_BYTE, reinterpret_cast<uint8_t *>(altitude_ref), 1);

    uint8_t gps_version_id[4] = {0x02, 0x03, 0x00, 0x00};
    new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_VERSION_ID, EXIF_FORMAT_BYTE, gps_version_id, 4);

    exif->ifd[EXIF_IFD_GPS] = gps_content;

    print_operation_time("get_exif");

#if 0
    int fd = open((std::string(path) + std::string("ori")).c_str(), O_CREAT | O_RDWR);
    if(fd < 0){
        printf("open jpeg original file failed\n");
    }else{
        write(fd, (void *)address, size);
        close(fd);
    }
    

    JPEGData *jpegData = jpeg_data_new_from_data((uint8_t *)address, size);
    if(exif != nullptr){
        jpeg_data_set_exif_data(jpegData, exif);
    }
    
    int ret = jpeg_data_save_file(jpegData, path);
    exif_data_free(exif);
    jpeg_data_free(jpegData);
#else
    print_operation_time("save_file_start");
    int ret = save_picture_with_exif(path, address, size, exif);
    print_operation_time("save_file_over");
    exif_data_unref(exif);
#endif
    return ret == 0;
}