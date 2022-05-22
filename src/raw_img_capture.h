#ifndef RAW_IMG_CAPTURE_H
#define RAW_IMG_CAPTURE_H

#include "frame_consumer.h"

#define RAW_PREFIX_STRING "img"
#define RAW_SUFFIX_STRING ".raw"

typedef void (*SavedCallback)(std::string path, void *data);

class RawImgCapture : public FrameConsumer
{
public:
    RawImgCapture(std::string path = ".");
    ~RawImgCapture();
    int start();
    int stop();
    void onFrameReceivedCallback(void *address, std::uint64_t size, void *extra_data);
    void setSavedCallback(SavedCallback cb, void *data = NULL);
    void setPath(std::string path);
private:
    SavedCallback onSavedCallback;
    void *onSavedCallbackData;
    std::string filePath;

};
#endif 