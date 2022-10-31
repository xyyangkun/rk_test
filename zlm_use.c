/******************************************************************************
 *
 *       Filename:  zlm_use.c
 *
 *    Description: zlm use 
 *
 *        Version:  1.0
 *        Created:  2021年06月08日 19时15分39秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include "mk_mediakit.h"
#include "zlm_use.h"

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
static mk_media zlm_video0 = NULL;
static mk_media zlm_video5;
static mk_player zlm_player; 
static mk_player player[5]; 
static mk_pusher _mk_PusherHandle;

static int is_init = 0;

// 初始化
// type == 0 h264 
// type == 1 h265
int zlm_init(int type, int frame_rate)
{
	// 保证zlm_init 只会初始化一次
#if 0
	if(is_init == 0) {
		is_init = 1;
	} else {
		return 0;
	}
#endif
	pthread_mutex_lock(&_mutex);
#if 1
    char *ini_path = mk_util_get_exe_dir("c_api.ini");
    char *ssl_path = mk_util_get_exe_dir("ssl.p12");

    mk_config config = {
            .ini = ini_path,
            .ini_is_path = 1,
            .log_level = 0,
			//.log_mask = LOG_CONSOLE,
            .log_file_path = NULL,
            .log_file_days = 0,
            .ssl = ssl_path,
            .ssl_is_path = 1,
            .ssl_pwd = NULL,
            .thread_num = 2
    };
    mk_env_init(&config);
    free(ini_path);
    free(ssl_path);
#endif

	//mk_rtsp_server_start(554, 0);
	//mk_rtmp_server_start(1935, 0);


	zlm_video0 = mk_media_create( \
				"__defaultVhost__", "live", "media0", \
				0, 0,  0
			);
	if(type == 0)
	{
		printf("=======================> mk_media_init_video h264\n");
		//mk_media_init_video(zlm_video0 , 0, 1920, 1080, 60);
		mk_media_init_video(zlm_video0 , 0, 1920, 1080, frame_rate);
	}
	else
	{
		printf("=========================> mk_media_init_video h265\n");
		//mk_media_init_video(zlm_video0 , 1, 1920, 1080, 60);
		mk_media_init_video(zlm_video0 , 1, 1920, 1080, frame_rate);
	}

	//mk_media_init_audio(zlm_video0 , 2, 48000, 2, 16); // aac

	//mk_media_init_audio(zlm_video0 , 2, 48000, 6, 16); // l16
	//mk_media_set_on_regist(_mk_MediaHandle, MediasourceRegistCB, this);
	mk_media_init_complete(zlm_video0);
	
	zlm_video5 = mk_media_create( \
				"__defaultVhost__", "live", "media1", \
				0, 0,  0
			);

	// h265
	mk_media_init_video(zlm_video5 , 1, 1920, 1080, frame_rate);


	pthread_mutex_unlock(&_mutex);
	printf("create rtsp server");

	return 0;
}

void zlm_pull_init_with_user_data(char *url, player_cb cb, void *user_data, int dec_chn)
{
	pthread_mutex_lock(&_mutex);
	player[dec_chn] = mk_player_create();
	mk_player_set_on_data(player[dec_chn], (on_mk_play_data)cb, user_data);
	mk_player_play(player[dec_chn], url); 
	pthread_mutex_unlock(&_mutex);
}
int zlm_pull_deinit_with_user_data(int dec_chn)
{
	pthread_mutex_lock(&_mutex);
	mk_player_release(player[dec_chn]);
	pthread_mutex_unlock(&_mutex);
}

int zlm_pull_init(char *url, player_cb cb)
{
	pthread_mutex_lock(&_mutex);
	zlm_player = mk_player_create();
	mk_player_set_on_data(zlm_player, (on_mk_play_data)cb, zlm_player);
	mk_player_play(zlm_player, url); 
	pthread_mutex_unlock(&_mutex);
}

int zlm_pull_deinit()
{
	pthread_mutex_lock(&_mutex);
	mk_player_release(zlm_player);
	pthread_mutex_unlock(&_mutex);
}

// 反初始化
int zlm_deinit()
{
	zlm_push_stop();

	pthread_mutex_lock(&_mutex);
	mk_media_release(zlm_video0);
	mk_media_release(zlm_video5);
	zlm_video0 = NULL;
	zlm_video5 = NULL;


	mk_stop_all_server();

	pthread_mutex_unlock(&_mutex);
	return 0;
}
// 送数据接口
int zlm_send0_h264(void *buf, unsigned int len, unsigned int dts, unsigned int pts)
{
	//printf("=====================>h264 video %p, buf:%p, len:%u, dts:%u, pts:%u\n", (void*)zlm_video0, buf, len, dts, pts);
	pthread_mutex_lock(&_mutex);
	if(zlm_video0) {
		mk_media_input_h264(zlm_video0, buf, len, dts, pts);
	}
	pthread_mutex_unlock(&_mutex);
	return 0;
}

// 送数据接口
int zlm_send0_h265(void *buf, unsigned int len, unsigned int dts, unsigned int pts)
{
	pthread_mutex_lock(&_mutex);
	//printf("=====================>h265 video %p, buf:%p, len:%u, dts:%u, pts:%u\n", (void*)zlm_video0, buf, len, dts, pts);
	if(zlm_video0){
		mk_media_input_h265(zlm_video0, buf, len, dts, pts);
	}
	pthread_mutex_unlock(&_mutex);
	return 0;
}

int zlm_send0_audio(void *buf, unsigned int len, unsigned int dts, void *adts)
{
	pthread_mutex_lock(&_mutex);
	//printf("=====================>audio %p, buf:%p, len:%u, dts:%u, adts:%p\n", (void*)zlm_video0, buf, len, dts, adts);
	if(zlm_video0) {
		mk_media_input_aac(zlm_video0, buf, len, dts, adts);
	}
	pthread_mutex_unlock(&_mutex);
	//mk_media_input_pcm(zlm_video0, buf, len, dts);
	return 0;
}

int zlm_send1(void *buf, unsigned int len, unsigned int dts, unsigned int pts)
{
	pthread_mutex_lock(&_mutex);
	mk_media_input_h264(zlm_video5, buf, len, dts, pts);
	pthread_mutex_unlock(&_mutex);
	return 0;
}

int zlm_record_start(const char *path, int max_second)
{
	//mk_set_option("mp4_record_path", "/mnt/11.mp4");
	printf("=====================>zlm_record_start path:%s max_second:%d\n", path, max_second);
	int ret = 0;
	pthread_mutex_lock(&_mutex);
	ret = mk_recorder_start(1, "__defaultVhost__", "live", "media0", path, max_second);
	if(ret != 1)
	{
		pthread_mutex_unlock(&_mutex);
		printf("ERROR to start record!\n");
		return -1;
	}
	pthread_mutex_unlock(&_mutex);
	return 0;
}

int zlm_record_stop()
{
	int ret = 0;
	pthread_mutex_lock(&_mutex);
	ret = mk_recorder_stop(1, "__defaultVhost__", "live", "media0");
	if(ret != 1)
	{
		pthread_mutex_unlock(&_mutex);
		printf("ERROR to stop record!\n");
		return -1;
	}
	pthread_mutex_unlock(&_mutex);
	return 0;
}


// 本地保存url
static char _url[1024];

// qt 设置推流状态
static int push_state = 0;
void re_push();
//	if(err_code != 0)
//	{
//		sleep(2);
//		re_push();
//	}

// 获取qt 设置的推流状态，
// qt 设置的推流状态和实际的的 rtmp push 状态是存在不同的概念，这两种状态也不一定相同
// 比如断网时推流，qt 设置的是推流状态，但实际rtmp push是失败
int get_setting_push_state(){return push_state;};

void API_CALL PushResultCB(void *user_data, int err_code, const char *err_msg)
{
	printf("err_code:%d\n", err_code);
	printf("err_msg:%s\n", err_msg);
	// 连接失败重置状态
	if(err_code != 0){
	}
	// 连接失败，重新连接
	if(err_code != 0 && push_state == 1){
	printf("err_code=%d, will repush!\n", err_code);
	}
}

void API_CALL PushShutdownCB(void *user_data, int err_code, const char *err_msg)
{
	printf("push shutdown cb:%d %s\n", err_code, err_msg);

	// 连接失败，重新连接
	if(err_code != 0 && push_state == 1){
	printf("err_code=%d, will repush!\n", err_code);
	}
}

int zlm_push_start(const char *url)
{
	strcpy(_url, url);
	pthread_mutex_lock(&_mutex);
	push_state = 1;
	 _mk_PusherHandle = mk_pusher_create("rtmp", "__defaultVhost__", "live", "media0");
	//_mk_PusherHandle = mk_pusher_create_src(zlm_video0);
	mk_pusher_set_on_result(_mk_PusherHandle, PushResultCB, NULL);
	mk_pusher_set_on_shutdown(_mk_PusherHandle, PushShutdownCB, NULL);
	mk_pusher_publish(_mk_PusherHandle, url);
	pthread_mutex_unlock(&_mutex);

	return 0;
}

int zlm_push_stop()
{
	pthread_mutex_lock(&_mutex);
	push_state = 0;
	if(_mk_PusherHandle)
	{
		mk_pusher_release(_mk_PusherHandle);
		_mk_PusherHandle = NULL;
	}
	pthread_mutex_unlock(&_mutex);
	return 0;
}

void re_push()
{
	if(_mk_PusherHandle)
	{
		mk_pusher_release(_mk_PusherHandle);
		_mk_PusherHandle = NULL;
	}

	_mk_PusherHandle = mk_pusher_create("rtmp", "__defaultVhost__", "live", "media0");
	//_mk_PusherHandle = mk_pusher_create_src(zlm_video0);
	mk_pusher_set_on_result(_mk_PusherHandle, PushResultCB, NULL);
	mk_pusher_set_on_shutdown(_mk_PusherHandle, PushShutdownCB, NULL);
	mk_pusher_publish(_mk_PusherHandle, _url);
}


// 获取rtmp推流实时网速， 推流状态
int zlm_get_push_state(int *speed, int *net_state)
{
#if 1
	pthread_mutex_lock(&_mutex);
	if(_mk_PusherHandle)
	{
		//printf("====================>yk debug get push state!\n");
		mk_pusher_get_state(_mk_PusherHandle, speed, net_state);
		pthread_mutex_unlock(&_mutex);
		return 0;
	}
	pthread_mutex_unlock(&_mutex);
#endif
	return -1;
}
