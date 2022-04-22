#ifndef _FRAME_CONSUMER_H
#define _FRAME_CONSUMER_H
#include <iostream>
class FrameConsumer
{
private:
    /* data */
public:
    FrameConsumer();
    ~FrameConsumer();
    static bool save_frame_to_file(const char *fn, void* addr, unsigned long size);
    virtual void onFrameReceivedCallback (void* address, std::uint64_t size) = 0;
};

#endif