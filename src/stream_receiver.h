#ifndef _STREAM_RECEIVER_H
#define _STREAM_RECEIVER_H
#include <vector>
#include <map>
#include "frame_consumer.h"

class StreamReceiver
{
public:
    StreamReceiver();
    ~StreamReceiver();

    void addConsumer(int id, FrameConsumer *consumer);
    void removeConsumer(FrameConsumer *consumer);
    int start();
    int stop();
    class StreamConsumer{
    public:
        StreamConsumer(int id, FrameConsumer* consumer);
        FrameConsumer *fConsumer;
        int streamId;
    };
    std::vector<StreamConsumer *> consumerList;
private:
};

#endif // !_STREAM_RECEIVER_H
