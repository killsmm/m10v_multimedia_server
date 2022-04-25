#include "frame_consumer.h"
#include <fstream>


bool FrameConsumer::save_frame_to_file(const char *fn, void* addr, unsigned long size) {
    std::cout << "file name = " << fn << std::endl;
    std::ofstream os;
    os.open(fn, std::ofstream::binary);
    os.write(static_cast<const char*>(addr), size);
    os.close();
    return os.good();
}

FrameConsumer::FrameConsumer() {
}

FrameConsumer::~FrameConsumer() {
    
}
