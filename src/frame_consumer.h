#ifndef _FRAME_CONSUMER_H
#define _FRAME_CONSUMER_H
#include <iostream>
class FrameConsumer
{
private:
    /* data */
public:
    FrameConsumer(int id);
    ~FrameConsumer();
    virtual int start() = 0;
    virtual int stop() = 0;
    virtual void onFrameReceivedCallback (void* address, std::uint64_t size) = 0;
    int streamId;
};

#endif