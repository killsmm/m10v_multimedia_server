#include "device_status.h"


uint32_t DeviceStatus::shutter_count = 0; 
float    DeviceStatus::cmos_temperature = 0; // degree
float    DeviceStatus::input_voltage = 0; 
int32_t  DeviceStatus::noise_reduction_strength = 0; // 0 - 4 low to high
int32_t  DeviceStatus::jpeg_quality_level = 0; // 0 - 4 low to high 0 for 70, 1 for 80, 2 for 90, 3 for 100
int32_t  DeviceStatus::shutter_mode = 0; // 0 for electrical , 1 for mechnical