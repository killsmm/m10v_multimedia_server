#include "json-c/json.h"
#include "boost/circular_buffer.hpp"

#define GPS_BUF_LENGTH 512

struct gps_data_t {
    float latitude;
    float longitude;
    float altitude;
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
    int timing_offset;
    boost::circular_buffer<struct gps_data_t> *gps_data_buf;
    ~GPSEstone();
};