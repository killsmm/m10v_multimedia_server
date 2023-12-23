#include "live555_server.h"
#include "sei_encoder.h"
#include "gps_estone.h"
#include "cif_stream.h"

Live555Server::Live555Server(std::string stream_name) {
    this->isStarted = false;
    this->scheduler = BasicTaskScheduler::createNew();
	this->env = BasicUsageEnvironment::createNew(*this->scheduler);	
    this->rtspServer = RTSPServer::createNew(*this->env);    

    OutPacketBuffer::maxSize = 8 * 1024 * 1024;

    this->serverMediaSession = ServerMediaSession::createNew(*this->env, stream_name.c_str());
    this->subSession = IPCU555Subsession::createNew(*this->env);
    this->serverMediaSession->addSubsession(this->subSession);
    // this->serverMediaSession = ServerMediaSession::createNew(*this->env, "sample");
    //H264VideoFileServerMediaSubsession *sub = H264VideoFileServerMediaSubsession::createNew(*this->env, "video_480x360_fps30.264", true);
    // this->serverMediaSession->addSubsession(sub);

    
}

Live555Server::~Live555Server() {

}



void Live555Server::onFrameReceivedCallback(void* address, std::uint64_t size, void *extra_data) {
    if(this->isStarted == false){
        this->start();
        return;
    }
    if(this->subSession->ipcuFramedSource != NULL){
        int sei_length = 0;
        uint8_t sei_data[128];
        struct gps_data_t gps_data;
        //TODO get the time stamp from the frame
        uint64_t frame_time_stamp = 0; 
        if(extra_data != nullptr){
            frame_time_stamp = (((uint32_t *)extra_data)[1] << 32 | ((uint32_t *)extra_data)[0]) / 90;
            //print extra data and frame time stamp
            printf("extra_data[0] = %d, extra_data[1] = %d, frame_time_stamp = %lld\n", ((uint32_t *)extra_data)[0], ((uint32_t *)extra_data)[1], frame_time_stamp);
        }
        GPSEstone::getInstance()->getGPSData(&gps_data, frame_time_stamp);
        SeiEncoder::encode(gps_data, sei_data, &sei_length);
        this->subSession->ipcuFramedSource->writeFrameToBuf((uint8_t *)address, size, sei_data, sei_length);
    }
}

void Live555Server::stop() {
    // pthread_join(this->thread, NULL);
    pthread_exit(&this->thread);
    this->isStarted = false;
}

void Live555Server::start() {
    this->isStarted = true;
    this->rtspServer->addServerMediaSession(this->serverMediaSession);
    pthread_create(&this->thread, NULL, [](void * data) -> void* { 
                        BasicUsageEnvironment *e = static_cast<BasicUsageEnvironment*>(data);
                        e->taskScheduler().doEventLoop();
                        }, (void *)this->env);
}
