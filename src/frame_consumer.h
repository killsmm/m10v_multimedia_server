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
    virtual void onFrameReceivedCallback (void* address, std::uint64_t size) = 0;
};

#endif