#ifndef SEI_ENCODER_H
#define SEI_ENCODER_H

#include <cstdint>
#include <sys/timeb.h>

#define SEI_BUF_LENGTH 64

class SeiEncoder{
public:
    static void encode(struct gps_data_t data, uint8_t *encoded_data, int *length); 
};

#endif // !SEI_ENCODER_H