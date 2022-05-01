#ifndef __CIF_STREAM_H__
#define __CIF_STREAM_H__

/**************************************************************** 
 *  include
 ****************************************************************/
#include "ipcu_userland.h"
#include "cmfwk_ipcu.h"
#include "dcf_if.h"
// Receive struct
#define CIF_STREAM_SEND_NAL_UNIT_SIZE_MAX 32

struct cif_stream_header
{
	unsigned long MagicCode;
	unsigned long Format_Version;
	unsigned long Command;
	unsigned long Sub_Command;
};

typedef struct
{
	unsigned long MagicCode;
	unsigned long Format_Version;
	unsigned long Command;
	unsigned long Sub_Command;
	unsigned long stype;
	unsigned long sformat;
	unsigned long stream_id;
	unsigned long stream_end_flg;
	unsigned long area_index;
	unsigned long sample_size;
	unsigned long sample_address;
	unsigned long sample_pts[2];
	unsigned long frame_no[2];
}cif_stream_meta_send;

typedef struct
{
        UINT64 sof;
        UINT64 eof;
}cif_sensor_timestamp;

struct cif_stream_send
{
	unsigned long MagicCode;
	unsigned long Format_Version;
	unsigned long Command;
	unsigned long Sub_Command;
	unsigned long stype;
	unsigned long sformat;
	unsigned long stream_id;
	unsigned long stream_end_flg;
	unsigned long area_index;
	unsigned long sample_size;
	unsigned long sample_address;
	union {
		struct cif_stream_send_video {
//			unsigned long reserved;
			unsigned long sample_pts[2];
			unsigned long sample_type;
			unsigned long frame_of_GOP;
			unsigned long nal_unit_num;
			unsigned long nal_unit_size[CIF_STREAM_SEND_NAL_UNIT_SIZE_MAX];
			unsigned long bitrate;
			unsigned long ave_bitrate;
			unsigned long width;
			unsigned long lines;
			cif_sensor_timestamp sensor_timestamp;
		}v;
		struct cif_stream_send_audio {
//			unsigned long reserved;
			unsigned long time_stamp[2];
			unsigned long channel;
			unsigned long sampling_freq;
			unsigned long bit_rate;
			unsigned long long frame_no;
		}a;
		struct cif_stream_send_raw {
			unsigned long reserved_ch0;
			unsigned long sample_pts[2];
			unsigned long global_width;
			unsigned long width;
			unsigned long lines;
			//unsigned long pack_type;
			//unsigned long first_pixel;
			unsigned long reserved_ch1;
			unsigned long long frame_no;
			cif_sensor_timestamp sensor_timestamp;
		}r;
		struct cif_stream_send_yuv {
			unsigned long reserved;
			unsigned long sample_pts[2];
			unsigned long width;
			unsigned long lines;
			unsigned long frame_no[2];
			cif_sensor_timestamp sensor_timestamp;
		}y;
		struct cif_stream_send_jpeg {
		//	unsigned long reserved;
			unsigned long sample_pts[2];
			unsigned long width;
			unsigned long lines;
			unsigned long ytbl_ofs;
			unsigned long ytbl_siz;
			unsigned long ctbl_ofs;
			unsigned long ctbl_siz;
			unsigned long frame_no[2];
			cif_sensor_timestamp sensor_timestamp;
			T_BF_DCF_IF_EXIF_INFO exif;
		}j;
	}media;
};

//Send struct
struct cif_stream_release {
	unsigned long MagicCode;
	unsigned long Format_Version;
	unsigned long Command;
	unsigned long Sub_Command;
	unsigned long stype;
	unsigned long sformat;
	unsigned long stream_id;
	unsigned long stream_end_flg;
	unsigned long area_index;
};
struct cif_stream_release_meta {
	unsigned long MagicCode;
	unsigned long Format_Version;
	unsigned long Command;
	unsigned long Sub_Command;
	unsigned long stype;
	unsigned long stream_id;
	unsigned long sample_address;
};
//When release the stream buffer, this uion's size must tell RTOS instead of cif_stream_release_meta and cif_stream_release's size
typedef union {
	struct cif_stream_release stream_release;
	struct cif_stream_release_meta meta_release;
} cif_release;

struct cb_main_func
{
	int (*video)(const struct cif_stream_send* p, void *d);
	int (*audio)(const struct cif_stream_send* p, void *d);
	int (*raw)(const struct cif_stream_send* p, void *d);
	int (*yuv)(const struct cif_stream_send* p, void *d);
	int (*jpeg)(const struct cif_stream_send* p, void *d);
	int (*meta)(const cif_stream_meta_send* p, void *d);
	int *user_data;
};
struct ipcu_param {
	int sw;
	int phys;
	int chid;
	int ackid;
};

struct cif_meta_non_shared_release {
	unsigned long MagicCode;
	unsigned long Format_Version;
	unsigned long Command_Set;
	unsigned long Command;
	unsigned long mtype;
	unsigned long meta_id;
	unsigned long frame_no[2];
};
struct cif_meta_shared_release {
	unsigned long MagicCode;
	unsigned long Format_Version;
	unsigned long Command_Set;
	unsigned long Command;
	unsigned long mtype;
	unsigned long data_address;
};

struct ipcu_meta_param {
	int sw;
	int phys;
};

struct camera_if_return {
	E_CAMERA_IF_RES_CODE ret;
	ULONG ret_code;
	ULONG recv_param[4];
};

struct camera_if_cmd {
	E_CAM_IF_COM_SET cmd_set;
	E_CAM_IF_SUB_COM cmd;
	ULONG send_param[4];
	ULONG recv_param[4];
	ULONG to_sec;
	ULONG dec_pos;
	int exp_time;
};

int Stream_ipcu_ch_init(struct cb_main_func* func, struct ipcu_param p);
int Stream_ipcu_ch_close(void);
int Stream_release(const void *p);
int Stream_release_with_queue(const void *p);
int Stream_release_set_count(const void *p, int conut);
int camera_if_command(struct camera_if_cmd *cam_if_cmd);

#endif// __CIF_STREAM_H__
