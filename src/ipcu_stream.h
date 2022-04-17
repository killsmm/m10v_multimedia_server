#ifndef _IPCU_STREAM_H
#define _IPCU_STREAM_H
#include "stdint.h"
#include "cif_stream.h"

#define IPCU_STREAM_SENDID 6
#define IPCU_STREAM_ACKID 7

typedef enum {
	DISP_IF = 0,
	CAG_EN,
	MOV_EN,
	MOV_AU_PORT,
	MOV_H265_TIMELAPSE,
	MOV_H264_0_TIMELAPSE,
	MOV_H264_1_TIMELAPSE,
	MOV_H265_SLOWMOTION,
	MOV_H264_0_SLOWMOTION,
	MOV_H264_1_SLOWMOTION,
	SENSOR_NUM,
	YUV_LAYOYT,
	USER_0_SIZE,
	USER_1_SIZE,
	H265_SIZE,
	H264_0_SIZE,
	H264_1_SIZE,
	H265_FPS,
	H264_0_FPS,
	H264_1_FPS,
	FACEDETECTION,
	PERSONDETECTION,
	VIEW_FPS,
	DOL_EN,
	H265_BITRATE,
	H264_0_BITRATE,
	H264_1_BITRATE,
	RAW_SELECT,
	YUV_SELECT,
	VIEW_YUV_FORMAT,
	SNAPSHOT_FORMAT,
	RAW_EXTRACT_POINT_FORCED,
	ROI_MODE,
	ZOOM_RATIO,
	ZOOM_POS_X,
	ZOOM_POS_Y,
	MIRROR_FLIP,
	ROTATE,
	LDC_MODE,
	STITCH_TYPE,
	OP_IPU_MODE,	// 0x28
	OP_3DNR_MODE,	// 0x29
	OP_EIS_MODE,	// 0x2A
	ROI0_ZOOM_RATIO,
	ROI0_ZOOM_POS_X,
	ROI0_ZOOM_POS_Y,
	MOV_H265_MAX_TIME,
	MOV_H264_0_MAX_TIME,
	MOV_H264_1_MAX_TIME,
	ROI1_ZOOM_RATIO,
	ROI1_ZOOM_POS_X,
	ROI1_ZOOM_POS_Y,
	ROI0_TRIM_RATIO,
	ROI1_TRIM_RATIO,
	ROI2_ZOOM_RATIO,
	ROI2_ZOOM_POS_X,
	ROI2_ZOOM_POS_Y,
	ROI2_TRIM_RATIO,
	AEAF_BASE_SIZE,	
	SHD_EN,
	LTM_MODE,
	SDC_EN,
	MOV_CACHE_EN,
	SEN_STREAM_MAPPING,
	MOVIE_DYNAMIC_UPDATE,
	MOVIE_GOP_NUM,
	AUTO_FRAMING,
	LOW_LATENCY_EN,
	LOW_LATENCY_LINE,
	SLICE_INTERRUPT_EN,
	ROI_MOVEMENT_TRIM_STEP,
	ROI_MOVEMENT_POS_X_STEP,
	ROI_MOVEMENT_POS_Y_STEP,
	ROI_MOVEMENT_COMP_FACTOR,
	ROI_MOVEMENT_MODE,
	AUTO_FRAMING_ROI_X,
	AUTO_FRAMING_ROI_Y,
	AUTO_FRAMING_ROI_W,
	AUTO_FRAMING_ROI_H,
    AUTO_FRAMING_PIP_MODE,
    AUTO_FRAMING_PIP_WIN_SIZE,
    AUTO_FRAMING_PIP_WIN_POS,
	ROI0_CONT_MODE,
	ROI0_CONT_DIR_PAN,
	ROI0_CONT_DIR_TILT,
	ROI0_CONT_DIR_ZOOM,
	MOV_H264_EXTEND_GOP_NUM,
	E_CATE01_MAX
} E_CATE01;	


struct Eptz_Params {
	float zoom_ratio;
	float x;
	float y;
};

typedef enum {
	 E_FJ_MOVIE_VIDEO_SIZE_4000_3000,
	 E_FJ_MOVIE_VIDEO_SIZE_4096_2304,
	 E_FJ_MOVIE_VIDEO_SIZE_4096_2160,
	 E_FJ_MOVIE_VIDEO_SIZE_4096_2048,
	 E_FJ_MOVIE_VIDEO_SIZE_3840_2160,
	 E_FJ_MOVIE_VIDEO_SIZE_3840_1920,
	 E_FJ_MOVIE_VIDEO_SIZE_3000_3000,
	 E_FJ_MOVIE_VIDEO_SIZE_2704_2028,
	 E_FJ_MOVIE_VIDEO_SIZE_2704_1520,
	 E_FJ_MOVIE_VIDEO_SIZE_2560_1920,
	 E_FJ_MOVIE_VIDEO_SIZE_1920_1440,
	 E_FJ_MOVIE_VIDEO_SIZE_1920_1080,
	 E_FJ_MOVIE_VIDEO_SIZE_1920_960,
	 E_FJ_MOVIE_VIDEO_SIZE_1504_1504,
	 E_FJ_MOVIE_VIDEO_SIZE_1440_1080,
	 E_FJ_MOVIE_VIDEO_SIZE_1440_720,
	 E_FJ_MOVIE_VIDEO_SIZE_1280_960,
	 E_FJ_MOVIE_VIDEO_SIZE_1280_720,
	 E_FJ_MOVIE_VIDEO_SIZE_960_480,
	 E_FJ_MOVIE_VIDEO_SIZE_800_600,
	 E_FJ_MOVIE_VIDEO_SIZE_864_480,
	 E_FJ_MOVIE_VIDEO_SIZE_848_480,
	 E_FJ_MOVIE_VIDEO_SIZE_720_480,
	 E_FJ_MOVIE_VIDEO_SIZE_640_480,
	 E_FJ_MOVIE_VIDEO_SIZE_640_360,
	 E_FJ_MOVIE_VIDEO_SIZE_432_240,
	 E_FJ_MOVIE_VIDEO_SIZE_320_240,
	 E_FJ_MOVIE_VIDEO_SIZE_720_576,
	 E_FJ_MOVIE_VIDEO_SIZE_1024_768,
	 E_FJ_MOVIE_VIDEO_SIZE_1280_768,
	 E_FJ_MOVIE_VIDEO_SIZE_1280_1024,
     E_FJ_MOVIE_VIDEO_SIZE_5472_3648,
	 E_FJ_MOVIE_VIDEO_SIZE_MAX
}RESOLUTION_INDEX;

typedef struct{
	RESOLUTION_INDEX index;
	int width;
	int height;
}S_RESOLUTION;

typedef int (*stream_callback)(const struct cif_stream_send *p, void *d);

int rtos_cmd(uint8_t cmd_set, uint8_t cmd, int param);
int init_stream(stream_callback cb, void *user_data);
void stop_stream();
int eptz(float zoom_ratio, float x, float y);
int eptz_gradually(struct Eptz_Params src, struct Eptz_Params dst);
int get_cur_eptz_params(struct Eptz_Params *params);
int set_ae_mode(int mode);
int get_ae_mode();
int set_face_detection(int enable);
int get_face_detection();
int set_ldc(int enable);
S_RESOLUTION get_resolution();
int set_wdr(unsigned char enable,
            unsigned char level,
            unsigned char contrast);
int set_iq_scene(unsigned char iq_scene);
int set_gamma_table(unsigned char table_num);
int reset_stream();
int get_rtos_version();
int finish_boot();
int start_preview();
int reboot_camera();
int set_3DNR(int enable);
int set_iso(int iso);
int set_ev_bias(float ev_bias);
int set_flicker(int flicker_mode);
int set_awb_style(int style);
int set_ae_lock(int enable);
int set_awb_lock(int enable);
int set_contrast(int value);
int set_jpeg_quality(int value);
int set_dol_hdr(int enable);
int set_resolution(uint8_t channel, int width, int height); 
#endif