#include "ipcu555_subsession.h"
#include "ipcu555_framed_source.h"


FramedSource* IPCU555Subsession::createNewStreamSource(unsigned clientSessionId,
                                                unsigned &estBitrate)
{
    estBitrate = 10000;
    // printf("createNewStreamSource \r\n");
    this->ipcuFramedSource = IPCU555FramedSource::createNew(envir(), nullptr);
    return H264VideoStreamDiscreteFramer::createNew(envir(), this->ipcuFramedSource);
}

RTPSink* IPCU555Subsession::createNewRTPSink(Groupsock *rtpGroupsock,
                                      unsigned char rtpPayloadTypeIfDynamic,
                                      FramedSource *inputSource)
{
    // printf("createNewRTPSink \r\n");
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}

IPCU555Subsession* IPCU555Subsession::createNew(UsageEnvironment& env,
                        char const* streamName)
{
    // printf("createNewRTPSink \r\n");
    return new IPCU555Subsession(env, streamName);
}

IPCU555Subsession::IPCU555Subsession(UsageEnvironment& env,
                        char const* streamName) : OnDemandServerMediaSubsession(env, true)
{
    
}
