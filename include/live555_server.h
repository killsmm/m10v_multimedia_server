#ifndef LIVE555_SERVER_H
#define LIVE555_SERVER_H

#include <string>
#include <pthread.h>
#include <ctime>
#include "frame_consumer.h"
#include <RTSPServer.hh>
#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <ServerMediaSession.hh>
#include "ipcu555_subsession.h"
#include <string>


typedef enum {
    LIVE555_SERVER_STATUS_PUSHING,
    LIVE555_SERVER_STATUS_READY,
}LIVE555_SERVER_STATUS;

class Live555Server : public FrameConsumer {

    public:
        Live555Server(std::string stream_name);
        ~Live555Server();
        void start();
        void stop();
        void onFrameReceivedCallback (void* address, std::uint64_t size, void *extra_data);
    private:
        std::string streamName;
        RTSPServer *rtspServer;
        pthread_spinlock_t spinLock;
        LIVE555_SERVER_STATUS serverStatus;
        BasicTaskScheduler *scheduler; 
        BasicUsageEnvironment *env;
        ServerMediaSession* serverMediaSession;
        IPCU555Subsession *subSession;
        pthread_t thread;
        bool isStarted;
};




#endif