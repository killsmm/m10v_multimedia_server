#ifndef DEVICE_STATUS_H
#define DEVICE_STATUS_H
#include <cstdint>
#include <iostream>

class DeviceStatus{
    public:
        static uint32_t shutter_count; 
        static float cmos_temperature; // degree
        static float input_voltage; 
        static int32_t noise_reduction_strength; // 0 - 4 low to high
        static int32_t jpeg_quality_level; // 0 - 4 low to high 0 for 70, 1 for 80, 2 for 90, 3 for 100
        static int32_t shutter_mode; // 0 for electrical , 1 for mechnical
        static std::string software_version;
        static std::string internal_code;
        static std::string serial_number;
};

#endif // !DEVICE_STATUS_H
