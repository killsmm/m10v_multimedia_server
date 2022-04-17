#include "stream_receiver.h"
#include "media_recorder.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <fcntl.h>

extern "C"{
#include "libavformat/avformat.h"
#include "cif_stream.h"
#include "ipcu_stream.h"
}

static int cb(const struct cif_stream_send* p, void *d){
    StreamReceiver* receiver = (StreamReceiver *)d;
    int ret = 0;
    for(FrameConsumer* consumer : receiver->consumers)
    {
        if (p->stream_id == consumer->streamId){
            consumer->onFrameReceivedCallback((void*)(p->sample_address), p->sample_size);
        }
    }
    return ret;
}

StreamReceiver::StreamReceiver() {

}

StreamReceiver::~StreamReceiver() {
    
}

void StreamReceiver::addConsumer(FrameConsumer * consumer) {
    consumers.push_back(consumer);
}

void StreamReceiver::removeConsumer(FrameConsumer *consumer) {
    for (int i = 0; i < consumers.size(); i++) {
        if (consumers[i] == consumer) {
            consumers.erase(consumers.begin() + i);
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
