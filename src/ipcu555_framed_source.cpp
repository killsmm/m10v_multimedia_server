#include "ipcu555_framed_source.h"
#include <pthread.h>
#include <iostream>
#include <queue>
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;



IPCU555FramedSource* IPCU555FramedSource::createNew(UsageEnvironment& env, void *data)
{
    return new IPCU555FramedSource(env, data);
}

int IPCU555FramedSource::writeFrameToBuf(uint8_t *addr, uint64_t size)
{
    
    if (size > BUFFER_SIZE) {
        return -1;
    }
    if (pthread_mutex_trylock(&lock) != 0){
        std::cout << "writeFrameToBuf trylock failed\n" << std::endl;
        return -1;
    }
    // std::cout << this->frame_size << " " << this->frame_buffer << std::endl;
    // printf("frame_buffer address: %08x\n", frame_buffer);
    memcpy(this->frame_buffer, addr, size);
    this->frame_size = size;
    pthread_mutex_unlock(&lock);
    
    return 0;
}

IPCU555FramedSource::IPCU555FramedSource(UsageEnvironment& env, void *data) 
        : FramedSource(env)
{
    this->frame_buffer = new uint8_t[BUFFER_SIZE];
    this->frame_size = 0;

}


void IPCU555FramedSource::doGetNextFrame()
{
    deliverFrame();
}

void IPCU555FramedSource::deliverFrame()
{
    // This function is called when new frame data is available from the device.
    // We deliver this data by copying it to the 'downstream' object, using the following parameters (class members):
    // 'in' parameters (these should *not* be modified by this function):
    //     fTo: The frame data is copied to this address.
    //         (Note that the variable "fTo" is *not* modified.  Instead,
    //          the frame data is copied to the address pointed to by "fTo".)
    //     fMaxSize: This is the maximum number of bytes that can be copied
    //         (If the actual frame is larger than this, then it should
    //          be truncated, and "fNumTruncatedBytes" set accordingly.)
    // 'out' parameters (these are modified by this function):
    //     fFrameSize: Should be set to the delivered frame size (<= fMaxSize).
    //     fNumTruncatedBytes: Should be set iff the delivered frame would have been
    //         bigger than "fMaxSize", in which case it's set to the number of bytes
    //         that have been omitted.
    //     fPresentationTime: Should be set to the frame's presentation time
    //         (seconds, microseconds).  This time must be aligned with 'wall-clock time' - i.e., the time that you would get
    //         by calling "gettimeofday()".
    //     fDurationInMicroseconds: Should be set to the frame's duration, if known.
    //         If, however, the device is a 'live source' (e.g., encoded from a camera or microphone), then we probably don't need
    //         to set this variable, because - in this case - data will never arrive 'early'.
    // Note the code below.
    if (pthread_mutex_trylock(&lock) != 0){
        std::cout << "doGetNextFrame trylock failed\n" << std::endl;
        return;
    }
    if (!isCurrentlyAwaitingData()){
        std::cout << "!isCurrentlyAwaitingData" << std::endl;
        return; // we're not ready for the data yet
    }
    u_int8_t *newFrameDataStart = this->frame_buffer; //%%% TO BE WRITTEN %%%
    unsigned newFrameSize = this->frame_size;                            //%%% TO BE WRITTEN %%%

    // Deliver the data here:
    if (newFrameSize > fMaxSize)
    {
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = newFrameSize - fMaxSize;
        std::cout << "newFrameSize > fMaxSize"  << newFrameSize << " > " << fMaxSize << std::endl;
    }
    else
    {
        fFrameSize = newFrameSize;
    }

    gettimeofday(&fPresentationTime, NULL);
    memmove(fTo, newFrameDataStart, fFrameSize);
    pthread_mutex_unlock(&lock);

    FramedSource::afterGetting(this);
}