#include "live555_server.h"
#include "sei_encoder.h"

Live555Server::Live555Server(std::string stream_name) {
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

    this->rtspServer->addServerMediaSession(this->serverMediaSession);
}

Live555Server::~Live555Server() {

}



void Live555Server::onFrameReceivedCallback(void* address, std::uint64_t size, void *extra_data) {
    if(this->subSession->ipcuFramedSource != NULL){
        int sei_length = 0;
        uint8_t *sei_data = SeiEncoder::getEncodedSei(&sei_length);
        this->subSession->ipcuFramedSource->writeFrameToBuf((uint8_t *)address, size, sei_data, sei_length);
    }
}

void Live555Server::stop() {
    pthread_exit(&this->thread);
}

void Live555Server::start() {
    pthread_create(&this->thread, NULL, [](void * data) -> void* { 
                        BasicUsageEnvironment *e = static_cast<BasicUsageEnvironment*>(data);
                        e->taskScheduler().doEventLoop();
                        }, (void *)this->env);
    
}
