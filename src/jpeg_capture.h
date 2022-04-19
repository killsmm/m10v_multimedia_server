#include "frame_consumer.h"

#define JPEG_PREFIX_STRING "img"
#define JPEG_SUFFIX_STRING ".jpg"

typedef void (*SavedCallback)(std::string path, void *data);

class JpegCapture : public FrameConsumer
{
public:
    JpegCapture(std::string path = "");
    ~JpegCapture();
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