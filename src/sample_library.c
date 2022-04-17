/******************************************************************************
 *
 *  @file   sample_library.c
 *  @brief  Sample Library for streaming with IPCU
 *
 *  Copyright 2017 Socionext Inc.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include "cmfwk_mm.h"
#include "cmfwk_ipcu.h"
#include "cmfwk_std.h"
#include "cif_stream.h"
#include <semaphore.h>


#define STREAM_QUEUE_SIZE           (128)
#define STREAM_COMMAND_MAX          (256)
#define RELEASE_STREAM_WAIT_TIMEOUT (3)

//#define DEBUG_BUFFER

static UINT8 res_id;
char  ts_id;
char phys_addr_on;
char phys_addr_meta_on;
char meta_res_id;
char meta_ts_id;

static volatile E_CAM_IF_COM_SET camera_if_current_cmd_set;
static volatile E_CAM_IF_SUB_COM camera_if_current_cmd;
static volatile UINT8 camera_if_cmd_send_id = 0xFF;
static volatile UINT8 camera_if_cmd_rcv_id = 0xFF;
static volatile struct camera_if_return camera_if_cmd_ret_value;

static pthread_mutex_t camera_if_cmd_mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t cb_sem;
static pthread_cond_t camera_if_cmd_cond = PTHREAD_COND_INITIALIZER;

struct cb_main_func func_p;

struct stream_queue {
	UINT8 command[STREAM_COMMAND_MAX];
	UINT32 length;
	int release_counter;
	struct stream_queue *next;
	struct stream_queue *prev;
};

static struct stream_queue *stream_pool = NULL;     /* stream_pool is LIFO */
static struct stream_queue *recv_queue = NULL;      /* recv_queue is FIFO */
static struct stream_queue *release_list = NULL;    /* release_list is FIFO */
static pthread_mutex_t stream_pool_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t recv_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t release_list_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t recv_stream_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t recv_stream_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t release_stream_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t release_stream_cond = PTHREAD_COND_INITIALIZER;

static pthread_mutex_t ipcu_send_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t stream_thread = 0;
#ifdef DEBUG_BUFFER
pthread_t debug_buffer_thread = 0;
#endif /* DEBUG_BUFFER */
int stop_stream_thread_flag = 0;

#ifdef DEBUG_BUFFER
static int stream_pool_count = 0;
static int recv_queue_count = 0;
static int release_list_count = 0;
#endif /* DEBUG_BUFFER */

#define CONV_SAMPLE_ADDRESS(x)	((x) = (phys_addr_on == 0) ? FJ_MM_phys_to_virt(x) : (x))

static void tty_printf(const char *format, ...)
{
	FILE *fp = fopen("/dev/ttyUSI0", "w+");
	va_list ap;

	va_start(ap, format);
	vfprintf(fp, format, ap);
	va_end(ap);

	fclose(fp);
}

static void recv_stream(const void *command)
{
	struct cif_stream_header *header = NULL;
	struct cif_stream_send *stream_send = NULL;
	cif_stream_meta_send *meta_send = NULL;
	header = (struct cif_stream_header*)command;

	if (header->MagicCode == 0x9999EEEE) {
		switch (header->Sub_Command) {
		/* streaming processing */
		case 0x0000100:
			stream_send = (struct cif_stream_send *)command;
			CONV_SAMPLE_ADDRESS(stream_send->sample_address);
			func_p.video(stream_send, func_p.user_data);
			break;
		case 0x0000200:
			stream_send = (struct cif_stream_send *)command;
			CONV_SAMPLE_ADDRESS(stream_send->sample_address);
			func_p.audio(stream_send, func_p.user_data);
			break;
		case 0x0000300:
			stream_send = (struct cif_stream_send *)command;
			CONV_SAMPLE_ADDRESS(stream_send->sample_address);
			func_p.raw(stream_send, func_p.user_data);
			break;
		case 0x0000400:
			stream_send = (struct cif_stream_send *)command;
			CONV_SAMPLE_ADDRESS(stream_send->sample_address);
			func_p.yuv(stream_send, func_p.user_data);
			break;
		case 0x0000500:
			stream_send = (struct cif_stream_send *)command;
			CONV_SAMPLE_ADDRESS(stream_send->sample_address);
			func_p.jpeg(stream_send, func_p.user_data);
			break;
		case 0x0010000:
			meta_send = (cif_stream_meta_send *)command;
			CONV_SAMPLE_ADDRESS(meta_send->sample_address);
			func_p.meta(meta_send, func_p.user_data);
			break;
		default:
			break;
		}
	}
	else {
		printf("[%s] Invalid MagicCode 0x%08lx\n", __func__, header->MagicCode);
	}
}

static int release_stream(const void *command)
{
	cif_release res_prm = 
	{
		.stream_release.MagicCode      = 0xAAAAFFFF,
		.stream_release.Format_Version = 0x00000001,
		.stream_release.Command        = 0x00010100,
	};
	int ret = -1;
	int len = 0;
	unsigned long par_for_res;
	struct cif_stream_header *header = (struct cif_stream_header *)command;

	if (header->MagicCode == 0x9999EEEE) {
		if (header->Sub_Command != 0x00010000) {
			/* Not meta */
			struct cif_stream_send *send_data = (struct cif_stream_send*)command;
			res_prm.stream_release.Sub_Command    = send_data->Sub_Command;
			res_prm.stream_release.stype          = send_data->stype;
			res_prm.stream_release.sformat        = send_data->sformat;
			res_prm.stream_release.stream_id      = send_data->stream_id;
			res_prm.stream_release.stream_end_flg = send_data->stream_end_flg;
			res_prm.stream_release.area_index     = send_data->area_index;
		}
		else {
			/* Meta */
			cif_stream_meta_send* send_data = (cif_stream_meta_send*)command;
			res_prm.meta_release.Sub_Command    = 0x00010000;
			res_prm.meta_release.stype          = send_data->stype;
			res_prm.meta_release.stream_id      = send_data->stream_id;
			res_prm.meta_release.sample_address = send_data->sample_address;
		}
		len = sizeof(res_prm);
		par_for_res = (unsigned long)(&res_prm);

		pthread_mutex_lock(&ipcu_send_mutex);
		ret = FJ_IPCU_Send(res_id, par_for_res, len, IPCU_SEND_SYNC);
		pthread_mutex_unlock(&ipcu_send_mutex);

		if (ret != 0) {
			printf("[%s] IPCU send error\n", __func__);
		}
#if 0
		printf("---send len %d---- \n",len);
		for (int i = 0; i < len/4; i++)
			printf("id=%02d  0x%08lX \n",i, *((unsigned long*)par_for_res + i));
#endif
	}
	return ret;
}


static void push_stream_pool(struct stream_queue *queue_data)
{
	struct stream_queue *pool = NULL;
	struct stream_queue *prev = NULL;

	pthread_mutex_lock(&stream_pool_mutex);

	prev = stream_pool;
	
	memset(queue_data, 0, sizeof(struct stream_queue));

	pool = queue_data;
	if (prev) {
		prev->prev = pool;
	}

	pool->next = prev;
	pool->prev = NULL;

	stream_pool = pool;

#ifdef DEBUG_BUFFER
	stream_pool_count++;
#endif /* DEBUG_BUFFER */

	pthread_mutex_unlock(&stream_pool_mutex);
}

static struct stream_queue *pop_stream_pool()
{
	struct stream_queue *pool = NULL;
	struct stream_queue *next = NULL;

	pthread_mutex_lock(&stream_pool_mutex);

	pool = stream_pool;

	if (pool) {
		next = pool->next;
		pool->prev = NULL;
		pool->next = NULL;
	}

	if (next) {
		next->prev = NULL;
	}

	stream_pool = next;

#ifdef DEBUG_BUFFER
	if (pool) {
		stream_pool_count--;
	}
#endif /* DEBUG_BUFFER */

	pthread_mutex_unlock(&stream_pool_mutex);

	return pool;
}

static void push_recv_queue(struct stream_queue *queue_data)
{
	struct stream_queue *queue = NULL;
	struct stream_queue *prev = NULL;

	pthread_mutex_lock(&recv_queue_mutex);

	if (recv_queue) {
		queue = recv_queue;
		while (queue->next) {
			queue = queue->next;
		}
		queue->next = queue_data;
		prev = queue;
	}
	else {
		recv_queue = queue_data;
	}

	queue = queue_data;
	queue->next = NULL;
	queue->prev = prev;

#ifdef DEBUG_BUFFER
	recv_queue_count++;
#endif /* DEBUG_BUFFER */

	pthread_mutex_unlock(&recv_queue_mutex);
}

static struct stream_queue *pop_recv_queue()
{
	struct stream_queue *queue = NULL;
	struct stream_queue *next = NULL;

	pthread_mutex_lock(&recv_queue_mutex);

	queue = recv_queue;

	if (queue) {
		next = queue->next;
		queue->prev = NULL;
		queue->next = NULL;
	}

	if (next) {
		next->prev = NULL;
	}

	recv_queue = next;

#ifdef DEBUG_BUFFER
	if (queue) {
		recv_queue_count--;
	}
#endif /* DEBUG_BUFFER */

	pthread_mutex_unlock(&recv_queue_mutex);

	return queue;
}

static void push_release_list(struct stream_queue *list_data)
{
	struct stream_queue *list = NULL;
	struct stream_queue *prev = NULL;

	pthread_mutex_lock(&release_list_mutex);

	if (release_list) {
		list = release_list;
		while (list->next) {
			list = list->next;
		}
		list->next = list_data;
		prev = list;
	}
	else {
		release_list = list_data;
	}

	list = list_data;
	list->next = NULL;
	list->prev = prev;

#ifdef DEBUG_BUFFER
	release_list_count++;
#endif /* DEBUG_BUFFER */

	pthread_mutex_unlock(&release_list_mutex);
}

static struct stream_queue *pop_release_list()
{
	struct stream_queue *list = NULL;
	struct stream_queue *next = NULL;

	pthread_mutex_lock(&release_list_mutex);

	list = release_list;

	if (list) {
		next = list->next;
		list->prev = NULL;
		list->next = NULL;
	}

	if (next) {
		next->prev = NULL;
	}

	release_list = next;

#ifdef DEBUG_BUFFER
	if (list) {
		release_list_count--;
	}
#endif /* DEBUG_BUFFER */

	pthread_mutex_unlock(&release_list_mutex);

	return list;
}

static void remove_release_list(struct stream_queue *list_data)
{
	struct stream_queue *next = NULL;
	struct stream_queue *prev = NULL;

	pthread_mutex_lock(&release_list_mutex);

	if (list_data) {
		next = list_data->next;
		prev = list_data->prev;
		list_data->prev = NULL;
		list_data->next = NULL;
	}

	if (next) {
		next->prev = prev;
	}

	if (prev) {
		prev->next = next;
	}
	else {
		release_list = next;
	}

#ifdef DEBUG_BUFFER
	if (list_data) {
		release_list_count--;
	}
#endif /* DEBUG_BUFFER */

	pthread_mutex_unlock(&release_list_mutex);
}

static struct stream_queue *search_release_list(const void *command)
{
	struct stream_queue *list = NULL;

	pthread_mutex_lock(&release_list_mutex);

	list = release_list;

	while (list) {
		if (list->command == command) {
			break;
		}
		list = list->next;
	}

	pthread_mutex_unlock(&release_list_mutex);

	return list;
}

static int initialize_stream_pool()
{
	int i;
	struct stream_queue *queue_data = NULL;

	if (stream_pool != NULL) {
		printf("[%s] stream pool has already been initialized\n", __func__);
		return -1;
	}
	if (recv_queue != NULL) {
		printf("[%s] receive queue has already been initialized\n", __func__);
		return -1;
	}
	if (release_list != NULL) {
		printf("[%s] release list has already been initialized\n", __func__);
		return -1;
	}

	for (i = 0; i < STREAM_QUEUE_SIZE; i++) {
		queue_data = malloc(sizeof(struct stream_queue));
		if (queue_data == NULL) {
			printf("[%s] stream queue malloc error\n", __func__);
			goto error;
		}
		push_stream_pool(queue_data);
	}

	return 0;

error:
	return -1;
}

static int finalize_stream_pool()
{
	struct stream_queue *queue_data = NULL;

	if (recv_queue != NULL) {
		printf("[%s] receive queue is still in use\n", __func__);
		return -1;
	}
	if (release_list != NULL) {
		printf("[%s] release list is still in use\n", __func__);
		return -1;
	}

	while (1) {
		queue_data = pop_stream_pool();
		if (queue_data == NULL) {
			break;
		}
		free(queue_data);
	}

	return 0;
}

static int enqueue_recv_command(const void *command, UINT32 length)
{
	int ret = 0;
	struct stream_queue *pool = NULL;

	pthread_mutex_lock(&recv_stream_mutex);

	if (length > sizeof(pool->command)) {
		printf("[%s] recv command size error (%d)\n", __func__, length);
		ret = -1;
		goto exit;
	}

	if (stop_stream_thread_flag) {
		ret = 1;
		goto exit;
	}

	pool = pop_stream_pool();
	if (pool == NULL) {
		ret = -1;
		goto exit;
	}

	memcpy(pool->command, command, length);
	pool->length = length;

	push_recv_queue(pool);

	pthread_cond_signal(&recv_stream_cond);

exit:
	pthread_mutex_unlock(&recv_stream_mutex);

	return ret;
}

static struct stream_queue *dequeue_recv_command()
{
	struct stream_queue *queue = NULL;

	pthread_mutex_lock(&recv_stream_mutex);

	while (queue == NULL) {
		queue = pop_recv_queue();
		if (queue == NULL) {
			if (!stop_stream_thread_flag) {
				pthread_cond_wait(&recv_stream_cond, &recv_stream_mutex);
			} else {
				goto exit;
			}
		}
	}

exit:
	pthread_mutex_unlock(&recv_stream_mutex);

	return queue;
}

static void store_release_command(struct stream_queue *queue)
{
	pthread_mutex_lock(&release_stream_mutex);

	queue->release_counter = 1; /* default : 1 time */
	push_release_list(queue);

	pthread_mutex_unlock(&release_stream_mutex);
}

static int check_release_counter(const void *command, struct stream_queue **list)
{
	int ret = 0;

	pthread_mutex_lock(&release_stream_mutex);

	*list = search_release_list(command);
	if (*list == NULL) {
		tty_printf("[%s] release command not found (%p)\n", __func__, command);
		ret = -1;
		goto exit;
	}

	if (--(*list)->release_counter > 0) {
		ret = (*list)->release_counter;
		goto exit;
	}

exit:
	pthread_mutex_unlock(&release_stream_mutex);

	return ret;
}

static void remove_release_command(struct stream_queue *list)
{
	pthread_mutex_lock(&release_stream_mutex);

	remove_release_list(list);
	push_stream_pool(list);

	pthread_cond_signal(&release_stream_cond);
	pthread_mutex_unlock(&release_stream_mutex);
}

static void force_remove_release_command()
{
	struct stream_queue *list = NULL;

	pthread_mutex_lock(&release_stream_mutex);

	while (1) {
		list = pop_release_list();
		if (list == NULL) {
			break;
		}
		push_stream_pool(list);
	}

	pthread_cond_signal(&release_stream_cond);
	pthread_mutex_unlock(&release_stream_mutex);
}

static int wait_release_list_empty()
{
	int ret = 0;
	struct timespec timeout;

	pthread_mutex_lock(&release_stream_mutex);

	timeout.tv_sec = time(NULL) + RELEASE_STREAM_WAIT_TIMEOUT;
	timeout.tv_nsec = 0;

	while (release_list) {
		ret = pthread_cond_timedwait(&release_stream_cond, &release_stream_mutex, &timeout);
		if (ret != 0) {
			break;
		}
	}

	pthread_mutex_unlock(&release_stream_mutex);

	return ret;
}

static int set_release_count(const void *command, int count)
{
	int ret = 0;
	struct stream_queue *list = NULL;

	pthread_mutex_lock(&release_stream_mutex);

	list = search_release_list(command);
	if (list == NULL) {
		tty_printf("[%s] release command not found (%p)\n", __func__, command);
		ret = -1;
		goto exit;
	}

	list->release_counter = count;

exit:
	pthread_mutex_unlock(&release_stream_mutex);

	return ret;
}

void* stream_thread_proc()
{
	struct stream_queue *queue = NULL;
	while (1) {
		queue = dequeue_recv_command();
		if (queue == NULL) {
			break;
		}
		store_release_command(queue);
		recv_stream(queue->command);
	};

	if (!wait_release_list_empty()) {
		force_remove_release_command();
	}

	return NULL;
}

#ifdef DEBUG_BUFFER
void* debug_buffer_thread_proc()
{
	struct stat stat_buf;
	while (!stop_stream_thread_flag) {
		if (stat("/tmp/debug_buff_count", &stat_buf) == 0) {
			int stream_pool;
			int recv_queue;
			int release_list;

			pthread_mutex_lock(&stream_pool_mutex);
			pthread_mutex_lock(&recv_queue_mutex);
			pthread_mutex_lock(&release_list_mutex);

			stream_pool = stream_pool_count;
			recv_queue = recv_queue_count;
			release_list = release_list_count;

			pthread_mutex_unlock(&release_list_mutex);
			pthread_mutex_unlock(&recv_queue_mutex);
			pthread_mutex_unlock(&stream_pool_mutex);

			tty_printf("[DEBUG] stream_pool = %4d, recv_queue = %4d, release_list = %4d\n",
				stream_pool, recv_queue, release_list);
		}
		if (remove("/tmp/debug_buff_detail") == 0) {
			struct stream_queue *list = release_list;

			pthread_mutex_lock(&release_list_mutex);

			tty_printf("[DEBUG] release_list_count = %d\n",
				release_list_count);

			while (list) {
				const struct cif_stream_header *header = list->command;

				tty_printf("[DEBUG] MagicCode = 0x%08X, Format_Version = 0x%08X, Command = 0x%08X, Sub_Command = 0x%08X\n",
					(UINT32)header->MagicCode, (UINT32)header->Format_Version, (UINT32)header->Command, (UINT32)header->Sub_Command);

				list = list->next;
			}

			pthread_mutex_unlock(&release_list_mutex);
		}
		sleep(1);
	}
	return NULL;
}
#endif /* DEBUG_BUFFER */

int start_stream_thread()
{
	int ret = -1;
	pthread_mutex_lock(&recv_stream_mutex);
	stop_stream_thread_flag = 0;
	pthread_mutex_unlock(&recv_stream_mutex);

	ret = pthread_create(&stream_thread, NULL, stream_thread_proc, (void *)NULL);
	if (ret) {
		stream_thread = 0;
	}

#ifdef DEBUG_BUFFER
	ret = pthread_create(&debug_buffer_thread, NULL, debug_buffer_thread_proc, (void *)NULL);
	if (ret) {
		debug_buffer_thread = 0;
	}
#endif /* DEBUG_BUFFER */

	return ret;
}

static void stop_stream_thread()
{
	if (stream_thread) {
		pthread_mutex_lock(&recv_stream_mutex);
		stop_stream_thread_flag = 1;
		pthread_cond_signal(&recv_stream_cond);
		pthread_mutex_unlock(&recv_stream_mutex);

		pthread_join(stream_thread, NULL);
		stream_thread = 0;
	}
#ifdef DEBUG_BUFFER
	if (debug_buffer_thread) {
		pthread_join(debug_buffer_thread, NULL);
		debug_buffer_thread = 0;
	}
#endif /* DEBUG_BUFFER */
}

static int initialize_stream_thread()
{
	int ret = -1;

	ret = initialize_stream_pool();
	if (ret) {
		goto exit;
	}

	ret = start_stream_thread();
	if (ret) {
		goto exit;
	}

exit:
	return ret;
}

static void finalize_stream_thread()
{
	stop_stream_thread();
	finalize_stream_pool();
}

int dummy_callback(struct cif_stream_send* p, void *d)
{
#if 0 
	int i;
	long *dbg = (void*)p;
	printf(" --- %s --- \n",__func__);
	//for( i= 0; i < sizeof( struct cif_stream_send ); i++ )
	for( i= 0; i < 64; i++ )
		printf("id=%02d  0x%08X \n",i, *((unsigned long*)dbg + i));
#endif
	return 0;
}

/******************************************************************************/
/**
 *  @brief  sample_app
 *  @fn void callback_camif(UINT8 id, UINT32 pdata, UINT32 length, UINT32 cont, UINT32 total_length)
 *  @param  [in]    id           : instance ID
 *  @param  [in]    pdata        : "Stream Send" format data top address
 *  @param  [in]    length       : byte size of data
 *  @param  [in]    cont         : continue flag: continue=1, no-continue(last)=0 for separeted communitcation by 32KB size
 *  @param  [in]    total_length : total length of data when separeted communitcation
 *  @note
 *  @author
 *  @version
 */
/******************************************************************************/
void callback_camif(UINT8 id,
		    UINT32 pdata,
		    UINT32 length,
		    UINT32 cont,
		    UINT32 total_length)
{
	void *buf_vaddr;
	struct cif_stream_send*  p;
	cif_stream_meta_send* mp;
	int ret = 1;
#if 0
	unsigned long par_for_res;
	int i;
	long *dbg = (void*)pdata;

	printf("[callback] IPCU#%d: buf=0x%08x len=%d cont=%d total_len=%d \n", id, pdata, length, cont, total_length);
	for( i= 0; i < sizeof( struct cif_stream_send ); i++ )
		printf("id=%02d  0x%08X \n",i, *((unsigned long*)dbg + i));
#endif

	p = (struct cif_stream_send*)pdata;
	if (phys_addr_on == 0) {
		if( p->Sub_Command != 0x00010000 )
		{
			buf_vaddr = (UINT32 *)FJ_MM_phys_to_virt(p->sample_address);
			p->sample_address = (UINT32)buf_vaddr;//Phys to Virt
		}
		else
		{
			mp = (cif_stream_meta_send*)(int)pdata;
			buf_vaddr = (UINT32 *)FJ_MM_phys_to_virt(mp->sample_address);
			mp->sample_address = (UINT32)buf_vaddr;//Phys to Virt
		}
	}

	if (p->MagicCode == 0x9999EEEE) {
		switch (p->Sub_Command) {
		/* streaming processing */
		case 0x0000100:
			ret = func_p.video(p, func_p.user_data);
			break;
		case 0x0000200:
			ret = func_p.audio(p, func_p.user_data);
			break;
		case 0x0000300:
			ret = func_p.raw(p, func_p.user_data);
			break;
		case 0x0000400:
			ret = func_p.yuv(p, func_p.user_data);
			break;
		case 0x0000500:
			ret = func_p.jpeg(p, func_p.user_data);
			break;
		case 0x0010000:
			if (phys_addr_on == 0) {
				mp = (cif_stream_meta_send*)(int)pdata;
				buf_vaddr = (UINT32 *)FJ_MM_phys_to_virt(mp->sample_address);
				mp->sample_address = (UINT32)buf_vaddr;//Phys to Virt
			}
			ret = func_p.meta((cif_stream_meta_send*)pdata, func_p.user_data);
            break;
		default:
			break;
		}
		if (ret > 0)
			return;
		/* response process */
		release_stream(p);

#if 0
		int i;
		printf("---send len %d---- \n",len);
		for (i= 0; i < len/4; i++)
			printf("id=%02d  0x%08lX \n",i, *((unsigned long*)par_for_res + i));
#endif
	}
	else
	{
		printf("[%s] Invalid MagicCode 0x%08lx\n",__func__,p->MagicCode);
	}
}

void callback_camif_async(UINT8 id,
		    UINT32 pdata,
		    UINT32 length,
		    UINT32 cont,
		    UINT32 total_length)
{
	static time_t last_logt_time = 0;
#if 0
	int i;
	long *dbg = (void*)pdata;

	printf("[callback] IPCU#%d: buf=0x%08x len=%d cont=%d total_len=%d \n", id, pdata, length, cont, total_length);
	for( i= 0; i < sizeof( struct cif_stream_send ); i++ )
		printf("id=%02d  0x%08X \n",i, *((unsigned long*)dbg + i));
#endif

	if (enqueue_recv_command((void *)pdata, length)) {
		release_stream((void *)pdata);

		time_t now = time(NULL);
		if ((last_logt_time + 1) < now) {
			tty_printf("This stream is forcibly released because there is no space in the buffer.\n");
			last_logt_time = now;
		}
	}
}

void callback_simple(UINT8 id,
		     UINT32 pdata,
		     UINT32 length,
		     UINT32 cont,
		     UINT32 total_length)
{
	static char   res_msg_ok[2] = {'O','K'};
	//static char   res_msg_ng[2] = {'N','G'};
	unsigned long *read_ptr, *write_ptr;
	struct cif_stream_send p;
	int ret = 0;

//	printf("%s[%d]",__func__,__LINE__);
	/* make dummy data */
	p.sample_size = length;
	p.sample_address = pdata;
	p.Sub_Command = 0x00000100;

	read_ptr = (unsigned long *)FJ_MM_phys_to_virt(0x4FE90048);
	write_ptr = (unsigned long *)FJ_MM_phys_to_virt(0x4FE9004C);

	func_p.video(&p, func_p.user_data);

	*read_ptr = *write_ptr;
	ret = FJ_IPCU_Send(res_id,
			   (UINT32)res_msg_ok,
			   sizeof(res_msg_ok),
			   IPCU_SEND_SYNC);
	if (ret != 0)
		printf("[%s] IPCU send error\n", __func__);
}

int Stream_ipcu_ch_init(struct cb_main_func* func, struct ipcu_param p)
{
	INT32  ret;
	INT32  chid = 6;
	INT32  ackid = 7;
	
	if (p.chid == p.ackid) {
		printf( "ERROR both ipcu ch \"%d\", \"%d\"",p.chid, p.ackid );
		chid = 6;
		ackid = 7;
	} else {
		if ((0 > p.chid && p.chid > 16) ||
		    (0 > p.ackid && p.ackid > 16 )) {
			printf("ERROR both ipcu ch \"%d\", \"%d\"",
						p.chid, p.ackid );
			chid = 6;
			ackid = 7;
		} else {
			chid = p.chid;
			ackid = p.ackid;
		}
	}

	if (p.phys == 1)
		phys_addr_on = 1;
	else
		phys_addr_on = 0;

	/* Preparation */
	ret = FJ_IPCU_Open(chid, (UINT8*)&ts_id);
	if (ret != FJ_ERR_OK)
		ret = -1;
	else {
		ret = FJ_IPCU_Open(ackid, &res_id);
		if (ret != FJ_ERR_OK) {
			ret = FJ_IPCU_Close(ts_id);
			ret = -1;
		} else {
			if (func != NULL) {
				if (func->video != NULL)
					func_p.video = (void*)func->video;
				else {
					func_p.video = (void*)dummy_callback;
					printf("%s %d video callback set dummy\n",
							__func__, __LINE__);
				}
				if (func->audio != NULL)
					func_p.audio = (void*)func->audio;
				else {
					func_p.audio = (void*)dummy_callback;
					printf("%s %d audio callback set dummy\n",
							__func__, __LINE__);
				}
				if (func->raw != NULL)
					func_p.raw   = (void*)func->raw;
				else {
					func_p.raw = (void*)dummy_callback;
					printf("%s %d raw callback set dummy\n",
							__func__, __LINE__);
				}
				if (func->yuv != NULL)
					func_p.yuv   = (void*)func->yuv;
				else {
					func_p.yuv = (void*)dummy_callback;
					printf("%s %d yuv callback set dummy\n",
							__func__, __LINE__);
				}
				if (func->jpeg != NULL)
					func_p.jpeg   = (void*)func->jpeg;
				else {
					func_p.jpeg = (void*)dummy_callback;
					printf("%s %d jpeg callback set dummy\n",
							__func__, __LINE__);
				}
				if (func->meta != NULL)
					func_p.meta   = (void*)func->meta;
				else {
					func_p.meta = (void*)dummy_callback;
					printf("%s %d meta callback set dummy\n",
							__func__, __LINE__);
				}
				func_p.user_data = func->user_data;

				if( p.sw == 2 ) {  /* async stream release mode */
					ret = initialize_stream_thread();
					if (ret == 0) {
						ret = FJ_IPCU_SetReceiverCB(
								ts_id,
								callback_camif_async);
					}
				}
				else if( p.sw == 1 ) /* not camera interface */
					ret = FJ_IPCU_SetReceiverCB(
							ts_id,
							callback_simple);
				else
					ret = FJ_IPCU_SetReceiverCB(
							ts_id,
							callback_camif);
			} else {
				printf("%s %d error NULL callback \n",
							__func__, __LINE__);
				ret = FJ_ERR_NG;
			}

			if (ret != FJ_ERR_OK) {
				ret = FJ_IPCU_Close(res_id);
				ret = FJ_IPCU_Close(ts_id);
				ret = -1;
			} else
				ret = 0;
		}
	}
	return ret;
}

int Stream_ipcu_ch_close(void)
{
	int  ret;

	/* Clean up */
	finalize_stream_thread();

	ret = FJ_IPCU_SetReceiverCB(ts_id, NULL);
	if (ret != FJ_ERR_OK)
		ret = -1;
	else {
		ret = FJ_IPCU_Close(res_id);
		if (ret != FJ_ERR_OK) {}
			/* DO NOTHING */

		ret = FJ_IPCU_Close(ts_id);
		if (ret != FJ_ERR_OK) {}
			/* DO NOTHING */
	}
	return ret;
}

int Stream_release(const void *p)
{
	return release_stream(p);
}

int Stream_release_with_queue(const void *p)
{
	int ret = -1;
	struct stream_queue *queue = NULL;

	ret = check_release_counter(p, &queue);
	if (ret == 0) {
		ret = release_stream(queue->command);
		remove_release_command(queue);
	}

	return ret;
}

int Stream_release_set_count(const void *p, int conut)
{
	int ret = 0;
	ret = set_release_count(p, conut);
	return ret;
}

// for Camera Interface --------------------------------
static void camera_if_cmd_rcv_callback(UINT8 id,
				       UINT32 pdata,
				       UINT32 length,
				       UINT32 cont,
				       UINT32 total_length)
{
	T_CPUCMD_COMMAND_ACK ack;

	//pthread_mutex_lock(&camera_if_cmd_mutex);

	if (camera_if_cmd_rcv_id != id ||
	    length != sizeof(ack) ||
	    length != total_length ||
	    cont == D_CONTINUE) {
		camera_if_cmd_ret_value.ret = E_CAMERA_IF_RES_CODE_ERROR;
		//pthread_cond_signal(&camera_if_cmd_cond);
		//pthread_mutex_unlock(&camera_if_cmd_mutex);
		sem_post(&cb_sem);
		printf("Receive response error...\n");
		return;
	}

	memcpy(&ack, (void*)pdata, sizeof(ack));

	if (ack.t_head.magic_code != D_CPU_IF_MCODE_COOMNAD_ACK ||
	    ack.t_head.format_version != D_CPU_IF_COM_FORMAT_VERSION ||
	    ack.t_head.cmd_set != camera_if_current_cmd_set ||
	    ack.t_head.cmd != camera_if_current_cmd) {
		camera_if_cmd_ret_value.ret = E_CAMERA_IF_RES_CODE_ERROR;
		printf("Response error. code=0x%08X-version=0x%08X-cmd_set=0x%08X-cmd=0x%08X\n",
			ack.t_head.magic_code,
			ack.t_head.format_version,
			ack.t_head.cmd_set,
			ack.t_head.cmd);
		//pthread_cond_signal(&camera_if_cmd_cond);
		//pthread_mutex_unlock(&camera_if_cmd_mutex);
		sem_post(&cb_sem);
		return;
	}

	camera_if_cmd_ret_value.ret = ack.ret1;
	camera_if_cmd_ret_value.ret_code = ack.ret2;
	camera_if_cmd_ret_value.recv_param[0] = ack.ret1;
	camera_if_cmd_ret_value.recv_param[1] = ack.ret2;
	camera_if_cmd_ret_value.recv_param[2] = ack.ret3;
	camera_if_cmd_ret_value.recv_param[3] = ack.ret4;
//	for(int i= 0; i < 4; i++){
//		printf("camera_if_cmd_ret_value.recv_param[%d] = %08x\n", i, camera_if_cmd_ret_value.recv_param[i]);
//	}
	//pthread_cond_signal(&camera_if_cmd_cond);
	//pthread_mutex_unlock(&camera_if_cmd_mutex);
	sem_post(&cb_sem);
	
	return;
}

static int camera_if_ipcu_ch_open(void)
{
	INT32 ret;

	/* Initialize IPCU */
	ret = FJ_IPCU_Open(E_FJ_IPCU_MAILBOX_TYPE_1,
			   (UINT8*)&camera_if_cmd_send_id);
	if (ret != FJ_ERR_OK)
		goto send_err;

	ret = FJ_IPCU_Open(E_FJ_IPCU_MAILBOX_TYPE_0,
			   (UINT8*)&camera_if_cmd_rcv_id);
	if (ret != FJ_ERR_OK)
		goto recv_err;

	ret = FJ_IPCU_SetReceiverCB(camera_if_cmd_rcv_id,
				    camera_if_cmd_rcv_callback);
	if (ret != FJ_ERR_OK)
		goto cam_if_cb_err;

	return 0;

cam_if_cb_err:
	FJ_IPCU_Close(camera_if_cmd_rcv_id);
recv_err:
	FJ_IPCU_Close(camera_if_cmd_send_id);
	camera_if_cmd_rcv_id = 0xFF;
send_err:
	camera_if_cmd_send_id = 0xFF;
	return -1;
}

static void camera_if_ipcu_ch_close(void)
{
	if (camera_if_cmd_send_id != 0xFF)
		FJ_IPCU_Close(camera_if_cmd_send_id);
	if (camera_if_cmd_rcv_id != 0xFF)
		FJ_IPCU_Close(camera_if_cmd_rcv_id);

	camera_if_cmd_send_id = 0xFF;
	camera_if_cmd_rcv_id = 0xFF;
}

static int camera_if_send_cmd(struct camera_if_cmd* cam_if_cmd)
{
	INT32  ret;
	T_CPUCMD_COMMAND_REQUEST cmd_req;
	struct camera_if_return tmp_tst;
    struct timespec timeout;

	memset(&tmp_tst, 0x00, sizeof(tmp_tst));
	camera_if_cmd_ret_value = tmp_tst;
	camera_if_current_cmd_set = cam_if_cmd->cmd_set;
	camera_if_current_cmd = cam_if_cmd->cmd;

	memset(&cmd_req, 0x0, sizeof(cmd_req));
	cmd_req.t_head.magic_code = D_CPU_IF_MCODE_COMMAND_REQ;
	cmd_req.t_head.format_version = D_CPU_IF_COM_FORMAT_VERSION;
	cmd_req.t_head.cmd_set = cam_if_cmd->cmd_set;
	cmd_req.t_head.cmd = cam_if_cmd->cmd;
	cmd_req.dec_pos = cam_if_cmd->dec_pos;
	cmd_req.exp_time = cam_if_cmd->exp_time;
	cmd_req.param1 = cam_if_cmd->send_param[0];
	cmd_req.param2 = cam_if_cmd->send_param[1];
	cmd_req.param3 = cam_if_cmd->send_param[2];
	cmd_req.param4 = cam_if_cmd->send_param[3];
	

	//pthread_mutex_lock(&camera_if_cmd_mutex);

	ret = sem_trywait(&cb_sem);
	if(ret != 0){
		printf("!!try wait failed!\n");
		goto end;
	}
	ret = FJ_IPCU_Send(camera_if_cmd_send_id,
			   (UINT32)&cmd_req, sizeof(cmd_req),
			   IPCU_SEND_SYNC);
	if (ret != FJ_ERR_OK) {

		goto end;
	}
#if 1
	timeout.tv_sec = time(NULL) + cam_if_cmd->to_sec;
	timeout.tv_nsec = 0;
//	switch (pthread_cond_timedwait(&camera_if_cmd_cond, &camera_if_cmd_mutex, &timeout)) {
	ret = sem_timedwait(&cb_sem, &timeout);
		switch (ret) {
			case 0:
				break;
			case EINTR:
				ret = -1;
				goto end;
			case ETIMEDOUT:
				ret = -2;
				goto end;
			default:
				ret = -1;
				goto end;
	}
	printf("write params\n");
	cam_if_cmd->recv_param[0] = camera_if_cmd_ret_value.recv_param[0];
	cam_if_cmd->recv_param[1] = camera_if_cmd_ret_value.recv_param[1];
	cam_if_cmd->recv_param[2] = camera_if_cmd_ret_value.recv_param[2];
	cam_if_cmd->recv_param[3] = camera_if_cmd_ret_value.recv_param[3];
#endif
end:
	//pthread_mutex_unlock(&camera_if_cmd_mutex);

	return ret;
}

int camera_if_command(struct camera_if_cmd* cam_if_cmd)
{
	int ret = 0;
	/* Open IPCU Camera IF channel */
	if (camera_if_ipcu_ch_open() != 0) {
		printf("camera_if_ipcu_open error.\n");
		ret = -1;
		goto end;
	}
	sem_init(&cb_sem, 0, 1);

	/* Send Camera IF */
	ret = camera_if_send_cmd(cam_if_cmd);
	if (ret != 0) {
		printf("camera_if_send_cmd error. ret=%d\n", ret);
		ret = -1;
	}
end:
	/* Close IPCU Camera IF channel */
	sem_destroy(&cb_sem);
	camera_if_ipcu_ch_close();
	return ret;
}

