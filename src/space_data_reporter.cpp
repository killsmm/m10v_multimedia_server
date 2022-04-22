#include "space_data_reporter.h"
#include <iostream>

SpaceDataReporter::SpaceData SpaceDataReporter::spaceData;
const int SpaceDataReporter::SPACE_DATA_SIZE = 64;

const SpaceDataReporter::SpaceData& SpaceDataReporter::getCurrentSpaceData() {
    spaceData.altitude = 1;
    spaceData.longitude = 2;
    spaceData.latitude = 3;
    spaceData.pan = 4;
    spaceData.tilt = 5;
    spaceData.roll = 6;
    return spaceData;
}

void SpaceDataReporter::writeCurrentSpaceDataToBuf(uint8_t *buf) {
    getCurrentSpaceData();
    *(uint64_t *)(buf + 8 * 0) = spaceData.latitude;
    *(uint64_t *)(buf + 8 * 1) = spaceData.longitude;
    *(uint64_t *)(buf + 8 * 2) = spaceData.altitude;
    *(uint64_t *)(buf + 8 * 3) = spaceData.pan;
    *(uint64_t *)(buf + 8 * 4) = spaceData.tilt;
    *(uint64_t *)(buf + 8 * 5) = spaceData.roll;
}

SpaceDataReporter::SpaceDataReporter() {
    
}
