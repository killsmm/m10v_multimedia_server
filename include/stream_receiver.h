#ifndef _STREAM_RECEIVER_H
#define _STREAM_RECEIVER_H
#include <vector>
#include <map>
#include "frame_consumer.h"


typedef enum {
	//STREAM_STR
  	E_CPU_IF_COMMAND_STREAM_VIDEO = 0x00000100,
	E_CPU_IF_COMMAND_STREAM_AUDIO = 0x00000200,
	E_CPU_IF_COMMAND_STREAM_RAW	  = 0x00000300,
	E_CPU_IF_COMMAND_STREAM_YUV = 0x00000400,
	E_CPU_IF_COMMAND_STREAM_JPG   = 0x00000500,
	E_CPU_IF_COMMAND_STREAM_META = 0x00010000,
} E_CPU_IF_COMMAND_STREAM;

class StreamReceiver
{
public:
    StreamReceiver();
    ~StreamReceiver();

    void addConsumer(E_CPU_IF_COMMAND_STREAM type, int id, FrameConsumer *consumer);
    void removeConsumer(FrameConsumer *consumer);
    int start();
    int stop();
    class StreamConsumer{
    public:
        StreamConsumer(E_CPU_IF_COMMAND_STREAM type, int id, FrameConsumer* consumer);
        FrameConsumer *fConsumer;
        int streamId;
        E_CPU_IF_COMMAND_STREAM streamType;
    };
    std::vector<StreamConsumer *> consumerList;
private:
};

#endif // !_STREAM_RECEIVER_H
