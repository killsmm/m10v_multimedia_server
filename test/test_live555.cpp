#include <gtest/gtest.h>
#include <liveMedia.hh>
#include "BasicUsageEnvironment.hh"



TEST(Live555, startRTSP){
        // Initialize the basic usage environment
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

    // Create a RTSP server
    RTSPServer* rtspServer = RTSPServer::createNew(*env, 8554);

    if (rtspServer == nullptr) {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
        return;
    }

    // Create a test RTSP stream
    ServerMediaSession* sms = ServerMediaSession::createNew(*env, "testStream", nullptr, "Session streamed by Live555", true);
    H264VideoFileServerMediaSubsession* subsession = H264VideoFileServerMediaSubsession::createNew(*env, "test.h264", true);
    sms->addSubsession(subsession);
    rtspServer->addServerMediaSession(sms);

    // Main loop
    env->taskScheduler().doEventLoop();

    // Clean up
    Medium::close(rtspServer);
    delete scheduler;

    return;
}