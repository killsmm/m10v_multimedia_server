#ifndef CONFIGS_H
#define CONFIGS_H

//video_recorder -s test -b "http://${REAL_IP}:${HTTP_PORT}/RAM_DCIM" -j "$RAM_DCIM" -v "$RAM_DCIM" > /dev/null &

#define DEFAULT_HTTP_PORT 8080
#define DEFAULT_VIDOE_RECORD_PATH ""
#define DEFUALT_JPEG_PATH ""
#define DEFAULT_RTSP_CHANNEL_NAME "test"
#define DEFAULT_RAM_DCIM_PATH "/tmp/ram_dcim"

#endif 
