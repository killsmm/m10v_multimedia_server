#ifndef SEI_ENCODER_H
#define SEI_ENCODER_H

#include <cstdint>

#define SEI_BUF_LENGTH 64

class SeiEncoder{
public:
    static uint8_t *getEncodedSei(int *length);
    static void init();
    static void deinit();
    static void setLocation(float latitude, float longitude, float altitude);
    static void setAngles(float pan, float pitch, float roll);
    static volatile float *longitude;
    static volatile float *latitude;
    static volatile float *altitude;
    static volatile float *roll;
    static volatile float *pitch;
    static volatile float *yaw; 
    static volatile uint8_t *encodedData;
    static uint8_t *addShellEncodedData;
};

#endif // !SEI_ENCODER_H