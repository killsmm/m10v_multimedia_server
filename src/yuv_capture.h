#ifndef YUV_CAPTURE_H
#define YUV_CAPTURE_H

#include "frame_consumer.h"

#define YUV_PREFIX_STRING "img"
#define YUV_SUFFIX_STRING ".raw"

typedef void (*SavedCallback)(std::string path, void *data);

class YuvCapture : public FrameConsumer
{
public:
    YuvCapture(std::string path = ".");
    ~YuvCapture();
    int start();
    int stop();
    void onFrameReceivedCallback(void *address, std::uint64_t size);
    void setSavedCallback(SavedCallback cb, void *data = NULL);
    void setPath(std::string path);
private:
    SavedCallback onSavedCallback;
    void *onSavedCallbackData;
    std::string filePath;

};

#endif // DEBUG

