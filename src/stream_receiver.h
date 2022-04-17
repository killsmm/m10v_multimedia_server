#ifndef _STREAM_RECEIVER_H
#define _STREAM_RECEIVER_H
#include <vector>
#include "frame_consumer.h"

class StreamReceiver
{
public:
    StreamReceiver();
    ~StreamReceiver();

    void addConsumer(FrameConsumer * consumer); 
    void removeConsumer(FrameConsumer *consumer);
    int start();
    int stop();
    std::vector<FrameConsumer *> consumers; 

private:
};

#endif // !_STREAM_RECEIVER_H
