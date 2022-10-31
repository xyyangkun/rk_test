/******************************************************************************
 *
 *       Filename:  zlm_use.h
 *
 *    Description:  zlm use
 *
 *        Version:  1.0
 *        Created:  2021年06月08日 19时14分03秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef _ZLM_USE_H_
#define _ZLM_USE_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define H264 0

// 初始化
int zlm_init(int type, int frame_rate);

// 反初始化
int zlm_deinit();

typedef struct s_zlm_user_data{
	char _buf[400*1024];
	int _len;
	int dec_chn;
}zlm_user_data;

typedef void player_cb(void *user_data, int track_type, int codec_id, void *data, int len, unsigned int dts, unsigned int pts);
void zlm_pull_init_with_user_data(char *url, player_cb cb, void *user_data, int dec_chn);
int zlm_pull_deinit_with_user_data(int dec_chn);
int zlm_pull_init(char *url, player_cb cb);
int zlm_pull_deinit();

// 送数据接口
int zlm_send0_h264(void *buf, unsigned int size, unsigned int dts, unsigned int pts);
int zlm_send0_h265(void *buf, unsigned int size, unsigned int dts, unsigned int pts);
int zlm_send1(void *buf, unsigned int len, unsigned int dts, unsigned int pts);

int zlm_send0_audio(void *buf, unsigned int len, unsigned int dts, void *adts);

// 获取数据接口


int zlm_record_start(const char *path, int max_second);
int zlm_record_stop();

int zlm_push_start(const char *url);
int zlm_push_stop();

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif//_ZLM_USE_H_
