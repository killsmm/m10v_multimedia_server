#ifndef __IPCU555_SUBSESSION_H__
#define __IPCU555_SUBSESSION_H__

#include <liveMedia.hh>
#include "ipcu555_framed_source.h"

class IPCU555Subsession : public OnDemandServerMediaSubsession
{
private:
    IPCU555Subsession(UsageEnvironment& env,
                        char const* streamName = NULL);
public:
    static IPCU555Subsession* createNew(UsageEnvironment& env,
                        char const* streamName = NULL);

    FramedSource *createNewStreamSource(unsigned clientSessionId,
                                                unsigned &estBitrate);
    RTPSink *createNewRTPSink(Groupsock *rtpGroupsock,
                                      unsigned char rtpPayloadTypeIfDynamic,
                                      FramedSource *inputSource);
    IPCU555FramedSource *ipcuFramedSource;

};



#endif // __IPCU555_SUBSESSION_H__