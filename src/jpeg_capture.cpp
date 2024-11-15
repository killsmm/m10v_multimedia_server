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
#include "gpio_ctrl.h"
#include "gps_estone.h"
// #include <arm_neon.h>
extern "C"{
    #include "jpeg-data.h"
}

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

JpegCapture::JpegCapture(std::string path){
    subPath = "";
    parentPath = path;
    std::cout << "JpegCapture::JpegCapture 1filePath = " << parentPath << std::endl;
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

static std::string get_url(std::string sub_dir, std::string file_name){
    return sub_dir + "/" + file_name;
}

static std::string get_file_name_now(std::string prefix, std::string suffix){
    struct timeval tv;
    time_t curtime;
    gettimeofday(&tv, NULL);
    curtime = tv.tv_sec;
    char time_str[100] = {0};
    strftime(time_str,100,"%y%m%d%H%M%S",localtime(&curtime));
    return prefix + std::string(time_str) + std::to_string(tv.tv_usec / 1000) + suffix; 

}

static std::string get_temp_file_name(){
    static int num = 0;
    if (num == JPEG_TEMPFILES_NUMBER){
        num = 0;
    }
    num++;
    return std::string("tmp") + std::to_string(num) + std::string(".jpg");
}

static std::string get_jpeg_full_path(std::string parent_path, std::string sub_path, std::string file_name){
    return parent_path + std::string("/") + sub_path + std::string("/") + file_name;
}



void JpegCapture::onFrameReceivedCallback(void* address, std::uint64_t size, void *extra_data) {
    if(pthread_mutex_trylock(&lock) != 0){
        std::cout << "jpeg try lock failed" << std::endl;
        return;
    }
    std::string file_name = get_temp_file_name();
    std::string full_path = get_jpeg_full_path(this->parentPath, "", file_name);
    std::cout << full_path << std::endl;
    T_BF_DCF_IF_EXIF_INFO *info = (T_BF_DCF_IF_EXIF_INFO*)(extra_data);
    if(saveJpegWithExif(address, size, *info, full_path.c_str()) == 0){
        //TODO add gpio status here
        // set_feedback_gpio(1);
        // usleep(5000); //5ms
        // set_feedback_gpio(0);
        if(onSavedCallback != NULL){
            onSavedCallback(get_url("", file_name), onSavedCallbackData);
        }
    }
    pthread_mutex_unlock(&lock);
}

void JpegCapture::setSavedCallback(SavedCallback cb, void* data){
    this->onSavedCallback = cb;
    this->onSavedCallbackData = data;
}

void JpegCapture::setSubPath(std::string path){
    this->subPath = path;
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
    ret.seconds = static_cast<uint32_t>(((decimal - ret.degrees) * 60 - ret.minutes) * 60);
    return ret;
}


static int save_picture_with_exif(const char* full_path, void *address, uint64_t ori_size, ExifData *exif, std::string xmp_str = ""){
    int ret = 0;
    uint32_t exif_length = 0;
    uint8_t *exif_data = NULL;

    std::string xmp_title = "http://ns.adobe.com/xap/1.0/";

    exif_data_save_data(exif, &exif_data, &exif_length);
    
    uint32_t jpeg_app_size = exif_length + 2;
    uint8_t jpeg_app_head[4] = {0xff, 0xe1};
    jpeg_app_head[2] = (jpeg_app_size & 0xff00) >> 8;
    jpeg_app_head[3] = jpeg_app_size & 0xff;

    uint8_t xmp_data_head[4] = {0xff, 0xe1, 0x00, 0x00};
    if (xmp_str != "") {
        uint32_t xmp_data_length = xmp_str.length() + xmp_title.length() + 1 + 2;
        xmp_data_head[2] = (xmp_data_length & 0xff00) >> 8;
        xmp_data_head[3] = xmp_data_length & 0xff;
    }

    


#if 1
    remove(full_path);
    int fd = open(full_path, O_CREAT | O_RDWR);
    if(fd < 0){
        printf("open jpeg original file failed\n");
    }else{
        ret |= write(fd, address, 2);
        ret |= write(fd, jpeg_app_head, 4);
        ret |= write(fd, exif_data, exif_length);
        ret |= write(fd, xmp_data_head, 4);
        ret |= write(fd, xmp_title.c_str(), xmp_title.length());
        ret |= write(fd, "\0", 1);
        ret |= write(fd, xmp_str.c_str(), xmp_str.length());
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
                                                .numerator = info.focal_length.nume,
                                                .denominator = info.focal_length.denomi
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

    struct gps_data_t gps_data;
    GPSEstone::getInstance()->getGPSData(&gps_data);

    uint8_t altitude_data[exif_format_get_size(EXIF_FORMAT_RATIONAL)] = {0};
    tmp.denominator = 1000;
    tmp.numerator = gps_data.altitude * 1000;
    exif_set_rational(altitude_data, byteOrder, tmp);
    new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_ALTITUDE, EXIF_FORMAT_RATIONAL, altitude_data, 1);

    uint8_t longitude_data[exif_format_get_size(EXIF_FORMAT_RATIONAL) * 3] = {0};
    GPS_DEGREE longi_degree = float_to_degree(gps_data.longitude);
    exif_set_rational(longitude_data, byteOrder, ExifRational{.numerator = longi_degree.degrees, .denominator = 1});
    exif_set_rational(longitude_data + exif_format_get_size(EXIF_FORMAT_RATIONAL) * 1, byteOrder, ExifRational{.numerator = longi_degree.minutes, .denominator = 1});
    exif_set_rational(longitude_data + exif_format_get_size(EXIF_FORMAT_RATIONAL) * 2, byteOrder, ExifRational{.numerator = longi_degree.seconds, .denominator = 1});
    new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_LONGITUDE, EXIF_FORMAT_RATIONAL, longitude_data, 3);

    uint8_t latitude_data[exif_format_get_size(EXIF_FORMAT_RATIONAL) * 3] = {0};
    GPS_DEGREE lati_degree = float_to_degree(gps_data.latitude);    
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

    // ExifRational pitch, roll, yaw = {0};
    // pitch.denominator = 1000;
    // pitch.numerator = gps_data.pitch * 1000;
    // roll.denominator = 1000;
    // roll.numerator = gps_data.roll * 1000;
    // yaw.denominator = 1000;
    // yaw.numerator = gps_data.yaw * 1000;
     
    // new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_, EXIF_FORMAT_RATIONAL, reinterpret_cast<uint8_t *>(&pitch), 1);
    // new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_ROLL, EXIF_FORMAT_RATIONAL, reinterpret_cast<uint8_t *>(&roll), 1);
    // new_exif_entry(gps_content, (ExifTag)EXIF_TAG_GPS_YAW, EXIF_FORMAT_RATIONAL, reinterpret_cast<uint8_t *>(&yaw), 1);



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
/*
    std::string xmp_info = "<?xpacket begin=\"\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>\n"
                        "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\" x:xmptk=\"Adobe XMP Core 5.4.0\">\n"
                        "  <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n"
                        "    <rdf:Description rdf:about=\"\" xmlns:exif=\"http://ns.adobe.com/exif/1.0/\""
                        "       share:Pitch=\"" + std::to_string(gps_data.yaw)  + "\""
                        "       share:Roll=\"" + std::to_string(gps_data.pitch) + "\""
                        "       share:Yaw=\"" + std::to_string(gps_data.roll) + "\""
                        "    />\n"
                        "  </rdf:RDF>\n"
                        "</x:xmpmeta>\n<?xpacket end=\"w\"?>\n";
*/
    

    std::string xmp_info = "<?xpacket begin=\"\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>\
    <x:xmpmeta xmlns:x=\"adobe:ns:meta/\" x:xmptk=\"XMP Core 4.4.0-Exiv2\">\
    <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\
        <rdf:Description rdf:about=\"DJI Meta Data\"\
            xmlns:share=\"http://www.shareuavtec.com/\"\
            xmlns:tiff=\"http://ns.adobe.com/tiff/1.0/\"\
            xmlns:drone-dji=\"http://www.dji.com/drone-dji/1.0/\"\
            share:Toolkit=\"XMP Core 4.4.0-Exiv2\"\
            share:SN=\"P102D220026\"\
            share:SoftwareVersion=\"1.058\"\
            share:DewarpData=\"\"\
            share:Lat=\"" + std::to_string(gps_data.latitude)  + "\"\
            share:Lon=\"" + std::to_string(gps_data.longitude)  + "\"\
            share:AbsAlt=\"" + std::to_string(gps_data.altitude)  + "\"\
            share:RltAlt=\"0\"\
            share:Pitch=\"" + std::to_string(gps_data.pitch)  + "\"\
            share:Roll=\"" + std::to_string(gps_data.roll)  + "\"\
            share:Yaw=\"" + std::to_string(gps_data.yaw)  + "\"\
            share:RTK=\"50\"\
            share:Perspectives=\"LOWER\"\
            share:WbMode=\"daylight\"\
            share:ColorMode=\"lively\"\
            share:DateTime=\"\"\
            tiff:Model=\"SHARE104S_X\"\
            drone-dji:DewarpFlag=\"0\"\
            drone-dji:DewarpData=\"\"\
            drone-dji:RtkFlag=\"50\"\
            drone-dji:RtkStdLon=\"0.0\"\
            drone-dji:RtkStdLat=\"0.0\"\
            drone-dji:RtkStdHgt=\"0.0\"\
            drone-dji:AbsoluteAltitude=\"" + std::to_string(gps_data.altitude)  + "\"\
            drone-dji:GpsLatitude=\"" + std::to_string(gps_data.latitude)  + "\"\
            drone-dji:GpsLongitude=\"" + std::to_string(gps_data.longitude)  + "\"\
            drone-dji:GimbalRollDegree=\"" + std::to_string(gps_data.roll)  + "\"\
            drone-dji:GimbalYawDegree=\"" + std::to_string(gps_data.yaw)  + "\"\
            drone-dji:GimbalPitchDegree=\"" + std::to_string(gps_data.pitch)  + "\"\
        />\
    </rdf:RDF>\
    </x:xmpmeta>\
    <?xpacket end=\"w\"?>";

/*
    std::string xmp_info = "<?xpacket begin=\"\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>\
<x:xmpmeta xmlns:x=\"adobe:ns:meta/\" x:xmptk=\"XMP Core 4.4.0-Exiv2\">\
  <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\
    <rdf:Description rdf:about=\"DJI Meta Data\"\
        xmlns:share=\"http://www.shareuavtec.com/\"\
        xmlns:tiff=\"http://ns.adobe.com/tiff/1.0/\"\
        xmlns:drone-dji=\"http://www.dji.com/drone-dji/1.0/\"\
        share:Toolkit=\"XMP Core 4.4.0-Exiv2\"\
        share:SN=\"P102D220026\"\
        share:SoftwareVersion=\"1.058\"\
        share:DewarpData=\"True,4,x,6144,4096,Perspective,Visible,23.1,25.39289665,-0.04584723,0.04138114,0.03775645,-0.00035709,-0.00016865,true,3119.38,2060.46,1,0,2022-06-08 11:50:34\"\
        share:Lat=\"40.35914417\"\
        share:Lon=\"117.13149927\"\
        share:AbsAlt=\"755.73093426\"\
        share:RltAlt=\"429.60848999\"\
        share:Pitch=\"-89.97763\"\
        share:Roll=\"63.42845\"\
        share:Yaw=\"-83.42845\"\
        share:RTK=\"50\"\
        share:Perspectives=\"LOWER\"\
        share:WbMode=\"daylight\"\
        share:ColorMode=\"lively\"\
        share:DateTime=\"2022-06-08 11:50:34\"\
        tiff:Model=\"SHARE104S_X\"\
        drone-dji:DewarpFlag=\"0\"\
        drone-dji:DewarpData=\"2022-06-08;6753.85,6753.85,47.37988281,12.45996094,-0.04584723,0.04138114,-0.00035709,-0.00016865,0.03775645\"\
        drone-dji:RtkFlag=\"50\"\
        drone-dji:RtkStdLon=\"0.01141\"\
        drone-dji:RtkStdLat=\"0.01247\"\
        drone-dji:RtkStdHgt=\"0.02413\"\
        drone-dji:AbsoluteAltitude=\"755.73093426\"\
        drone-dji:GpsLatitude=\"40.35914417\"\
        drone-dji:GpsLongitude=\"117.13149927\"\
        drone-dji:GimbalRollDegree=\"63.42845\"\
        drone-dji:GimbalYawDegree=\"-83.42845\"\
        drone-dji:GimbalPitchDegree=\"-89.97763\"\
    />\
  </rdf:RDF>\
</x:xmpmeta>\
<?xpacket end=\"w\"?>";
*/
    int ret = save_picture_with_exif(path, address, size, exif, xmp_info);
    print_operation_time("save_file_over");
    exif_data_unref(exif);
#endif
    return ret == 0;
}