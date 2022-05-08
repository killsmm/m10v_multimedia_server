#ifndef JPEG_CAPTURE_H
#define JPEG_CAPTURE_H

#include "frame_consumer.h"
#include "dcf_if.h"

#define JPEG_PREFIX_STRING "img"
#define JPEG_SUFFIX_STRING ".jpg"

typedef void (*SavedCallback)(std::string path, void *data);

class JpegCapture : public FrameConsumer
{
public:
    JpegCapture(std::string path = ".");
    ~JpegCapture();
    int start();
    int stop();
    void onFrameReceivedCallback(void *address, std::uint64_t size, void *extra_data);
    void setSavedCallback(SavedCallback cb, void *data = NULL);
    void setPath(std::string path);
    bool saveJpegWithExif(void *address, std::uint64_t size, T_BF_DCF_IF_EXIF_INFO info, const char *path);
private:
    SavedCallback onSavedCallback;
    void *onSavedCallbackData;
    std::string filePath;

};

#endif