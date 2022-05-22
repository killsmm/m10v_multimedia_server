#include "frame_consumer.h"
// #include <fstream>
#include <fcntl.h>
#include <unistd.h>

bool FrameConsumer::save_frame_to_file(const char *fn, void* addr, unsigned long size) {
    std::cout << "file name = " << fn << std::endl;
    // std::ofstream os;
    // os.open(fn, std::ofstream::binary);
    // os.write(static_cast<const char*>(addr), size);
    // os.close();
    // return os.good();
    int fd = open(fn, O_CREAT | O_RDWR);
    if(fd < 0){
        return -1;
    }
    int ret = write(fd, addr, size);
    return ret == size;
}

FrameConsumer::FrameConsumer() {
}

FrameConsumer::~FrameConsumer() {
    
}
