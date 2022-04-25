#include "stream_receiver.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <fcntl.h>
#include <cstring>
extern "C"{
#include "cif_stream.h"
#include "ipcu_stream.h"

}

static int cb(const struct cif_stream_send* p, void *d){
    StreamReceiver* receiver = (StreamReceiver *)d;
    int ret = 0;
    // // printf("sub_cmd = %08x, stream_id = %d\n",p->Sub_Command, p->stream_id);
    // printf("samplesize = %d\n", p->sample_size);
    for(StreamReceiver::StreamConsumer *c : receiver->consumerList)
    {
        // printf("c->streamType = %08x, stream_id = %d\n", c->streamType, c->streamId);
        if (c->streamType == p->Sub_Command && p->stream_id == c->streamId){
            c->fConsumer->onFrameReceivedCallback((void*)(p->sample_address), p->sample_size);
        }
    }
    return ret;
}

StreamReceiver::StreamReceiver() {

}

StreamReceiver::~StreamReceiver() {
    
}

StreamReceiver::StreamConsumer::StreamConsumer(E_CPU_IF_COMMAND_STREAM type, int id, FrameConsumer* consumer){
    streamId = id;
    fConsumer = consumer;
    streamType = type;
}

void StreamReceiver::addConsumer(E_CPU_IF_COMMAND_STREAM type, int id, FrameConsumer *consumer) {
    consumerList.push_back(new StreamConsumer(type, id, consumer));
}

void StreamReceiver::removeConsumer(FrameConsumer *consumer) {
    for(int i = 0; i < consumerList.size(); i++)
    {
        if(consumerList.at(i)->fConsumer == consumer){
            delete consumerList.at(i);
            consumerList.erase(consumerList.begin() + i);
        }
    }
}

int StreamReceiver::start() {
    struct cb_main_func func_p;
	struct ipcu_param ipcu_prm;
	memset(&func_p, 0x00, sizeof(func_p));
	func_p.jpeg = cb;
	func_p.video = cb;
	func_p.yuv = cb;
	func_p.audio = cb;
	func_p.meta = NULL;
	func_p.raw = cb;
    func_p.user_data = (int*)this;
	memset(&ipcu_prm, 0x00, sizeof(ipcu_prm));
	ipcu_prm.chid = IPCU_STREAM_SENDID;
	ipcu_prm.ackid = IPCU_STREAM_ACKID;
	return Stream_ipcu_ch_init(&func_p, ipcu_prm);
}

int StreamReceiver::stop() {
    Stream_ipcu_ch_close();
}
