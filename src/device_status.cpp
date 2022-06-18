#include "device_status.h"


uint32_t DeviceStatus::shutter_count = 55; 
float    DeviceStatus::cmos_temperature = 45.0; // degree
float    DeviceStatus::input_voltage = 12.0; 
int32_t  DeviceStatus::noise_reduction_strength = 0; // 0 - 4 low to high
int32_t  DeviceStatus::jpeg_quality_level = 2; // 0 - 3 low to high 0 for 80-89, 1 for 90-99, 2 for 100
int32_t  DeviceStatus::shutter_mode = 0; // 0 for electrical , 1 for mechnical
std::string DeviceStatus::software_version = "0.1.2";
std::string DeviceStatus::internal_code = "A";
std::string DeviceStatus::serial_number = "feituo";