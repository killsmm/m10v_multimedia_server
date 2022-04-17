#include "ipcu_stream.h"
#include "string.h"
#include "stdlib.h"

static S_RESOLUTION resolution_form[] = {
	{E_FJ_MOVIE_VIDEO_SIZE_4000_3000, .width = 4000, .height = 3000},
	{E_FJ_MOVIE_VIDEO_SIZE_4096_2304, .width = 4096, .height = 2304},
	{E_FJ_MOVIE_VIDEO_SIZE_4096_2160, .width = 4096, .height = 2160},
	{E_FJ_MOVIE_VIDEO_SIZE_4096_2048, .width = 4096, .height = 2048},
	{E_FJ_MOVIE_VIDEO_SIZE_3840_2160, .width = 3840, .height = 2160},
	{E_FJ_MOVIE_VIDEO_SIZE_3840_1920, .width = 3840, .height = 1920},
	{E_FJ_MOVIE_VIDEO_SIZE_3000_3000, .width = 3000, .height = 3000},
	{E_FJ_MOVIE_VIDEO_SIZE_2704_2028, .width = 2704, .height = 2028},
	{E_FJ_MOVIE_VIDEO_SIZE_2704_1520, .width = 2704, .height = 1520},
	{E_FJ_MOVIE_VIDEO_SIZE_2560_1920, .width = 2560, .height = 1920},
	{E_FJ_MOVIE_VIDEO_SIZE_1920_1440, .width = 1920, .height = 1440},
	{E_FJ_MOVIE_VIDEO_SIZE_1920_1080, .width = 1920, .height = 1080},
	{E_FJ_MOVIE_VIDEO_SIZE_1920_960, .width = 1920, .height = 960},
	{E_FJ_MOVIE_VIDEO_SIZE_1504_1504, .width = 1504, .height = 1504},
	{E_FJ_MOVIE_VIDEO_SIZE_1440_1080, .width = 1440, .height = 1080},
	{E_FJ_MOVIE_VIDEO_SIZE_1440_720, .width = 1440, .height = 720},
	{E_FJ_MOVIE_VIDEO_SIZE_1280_960, .width = 1280, .height = 960},
	{E_FJ_MOVIE_VIDEO_SIZE_1280_720, .width = 1280, .height = 720},
	{E_FJ_MOVIE_VIDEO_SIZE_960_480, .width = 960, .height = 480},
	{E_FJ_MOVIE_VIDEO_SIZE_800_600, .width = 800, .height = 600},
	{E_FJ_MOVIE_VIDEO_SIZE_864_480, .width = 864, .height = 480},
	{E_FJ_MOVIE_VIDEO_SIZE_848_480, .width = 848, .height = 480},
	{E_FJ_MOVIE_VIDEO_SIZE_720_480, .width = 720, .height = 480},
	{E_FJ_MOVIE_VIDEO_SIZE_640_480, .width = 640, .height = 480},
	{E_FJ_MOVIE_VIDEO_SIZE_640_360, .width = 640, .height = 360},
	{E_FJ_MOVIE_VIDEO_SIZE_432_240, .width = 432, .height = 240},
	{E_FJ_MOVIE_VIDEO_SIZE_320_240, .width = 320, .height = 240},
	{E_FJ_MOVIE_VIDEO_SIZE_720_576, .width = 720, .height = 576},
	{E_FJ_MOVIE_VIDEO_SIZE_1024_768, .width = 1024, .height = 768},
	{E_FJ_MOVIE_VIDEO_SIZE_1280_768, .width = 1280, .height = 768},
	{E_FJ_MOVIE_VIDEO_SIZE_1280_1024, .width = 1280, .height = 1024},
	{E_FJ_MOVIE_VIDEO_SIZE_5472_3648, .width = 5472, .height = 3648}
};

int rtos_cmd(uint8_t cmd_set, uint8_t cmd, int param){
	struct camera_if_cmd cam_if_cmd;
	printf("rtos_cmd %02x %02x %02x\n", cmd_set, cmd, param);
	memset((void*)&cam_if_cmd, 0x00, sizeof(cam_if_cmd));
	cam_if_cmd.cmd_set = cmd_set;
	cam_if_cmd.cmd = cmd;
	cam_if_cmd.send_param[0] = param;
	cam_if_cmd.to_sec = 3;
	camera_if_command(&cam_if_cmd);
	return cam_if_cmd.recv_param[0];
}

int init_stream(stream_callback cb, void *user_data){
	struct cb_main_func func_p;
	struct ipcu_param ipcu_prm;
	memset(&func_p, 0x00, sizeof(func_p));
	func_p.jpeg = cb;
	func_p.video = cb;
	func_p.yuv = cb;
	func_p.audio = cb;
	func_p.meta = NULL;
	func_p.raw = cb;
    func_p.user_data = (int*)(user_data);
	memset(&ipcu_prm, 0x00, sizeof(ipcu_prm));
	ipcu_prm.chid = IPCU_STREAM_SENDID;
	ipcu_prm.ackid = IPCU_STREAM_ACKID;
	return Stream_ipcu_ch_init(&func_p, ipcu_prm);
}

void stop_stream(){
	rtos_cmd(0x00, 0x0b, 0x09);
    Stream_ipcu_ch_close();
}

int finish_boot(){
	return rtos_cmd(0x0, 0x0, 0x3);
}

int start_preview(){
	return rtos_cmd(0x0, 0xb, 0x2);
}

int start_record(){
	return rtos_cmd(0x0, 0xb, 0x8);
}

int finish_record(){
	return rtos_cmd(0x0, 0xb, 0x9);
}

int capture_pic(){
	return rtos_cmd(0x0, 0xb, 0x3);
}

int reset_stream(){
	return rtos_cmd(0x0, 0xb, 0x2);
}

int set_resolution(uint8_t channel, int width, int height) {
	printf("channel = %d\n", channel);
	if (channel < USER_0_SIZE || channel > H264_1_SIZE) {
		return -1;
	}
	int dim = width + height;
	int result_index = 0xb;
	for (int i = 0; i < E_FJ_MOVIE_VIDEO_SIZE_MAX; i++) {
		if (resolution_form[i].width == width && resolution_form[i].height == height) {
			result_index = i;
			break;
		}
		
		if (dim > abs(resolution_form[i].width - width) + abs(resolution_form[i].height - height)) {
			dim = abs(resolution_form[i].width - width) + abs(resolution_form[i].height - height);
			result_index = i;
		}
	}
	printf("set resolution channel %d: %d x %d\n", channel, resolution_form[result_index].width, resolution_form[result_index].height);
	return rtos_cmd(0x1, channel, result_index);
}

int set_jpeg_quality(int value){
	if(value > 100 || value < 0){
		return -1;
	}
	return rtos_cmd(0xb, 0x6, value);
}