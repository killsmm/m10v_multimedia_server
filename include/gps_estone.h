#include "json-c/json.h"
#include "boost/circular_buffer.hpp"
#include <mutex>

#define GPS_BUF_LENGTH 64

struct gps_data_t {
    int32_t latitude;
    int32_t longitude;
    int32_t altitude;
    float roll;
    float pitch;
    float yaw;
    uint64_t time_stamp;
};

class GPSEstone{
public:
    static GPSEstone *getInstance();
    int handleData(json_object *obj);
    int handleData(struct gps_data_t *data);
    int getGPSData(gps_data_t *data, uint64_t time_stamp = 0);
    void setTimingOffset(int offset);
    int getTimingOffset();
private:
    GPSEstone();
    std::mutex dataMutex;
    int timing_offset;
    boost::circular_buffer<struct gps_data_t> *gps_data_buf;
    ~GPSEstone();
};