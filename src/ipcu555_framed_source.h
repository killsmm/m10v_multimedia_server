#ifndef __IPCU555_FRAMED_SOURCE_H__
#define __IPCU555_FRAMED_SOURCE_H__

#include <liveMedia.hh>
#include <vector>

#define BUFFER_SIZE (200 * 1024)
#define BUFFER_NB   3



class IPCU555FramedSource : public FramedSource{
private:
    void deliverFrame();
    IPCU555FramedSource(UsageEnvironment& env, void *data);
    uint8_t *frame_buffer;
    uint64_t frame_size;
    /* data */
public:
    // IPCU555FramedSource();
    // ~IPCU555FramedSource();

    static IPCU555FramedSource* createNew(UsageEnvironment& env, void *data);
    int writeFrameToBuf(uint8_t *addr, uint64_t size);
    void doGetNextFrame();
};


#endif // __IPCU555_FRAMED_SOURCE_H__