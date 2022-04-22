#ifndef _SPACE_DATA_REPORT_H
#define _SPACE_DATA_REPORT_H

#include <cstdint>


class SpaceDataReporter
{
public:
    class SpaceData {
    public:
        uint64_t longitude;
        uint64_t latitude;
        uint64_t altitude;
        uint64_t pan;
        uint64_t tilt;
        uint64_t roll;
    };
    static const SpaceData& getCurrentSpaceData();
    static void writeCurrentSpaceDataToBuf(uint8_t * buf);
    static const int SPACE_DATA_SIZE;

private:
    SpaceDataReporter();
    static SpaceData spaceData;
};

#endif // !_SPACE_DATA_REPORT_H
