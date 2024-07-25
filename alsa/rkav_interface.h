/******************************************************************************
 *
 *       Filename:  rkav_interface.h
 *
 *    Description:  rk3568 音视频控制接口
 *
 *        Version:  1.0
 *        Created:  2022年03月26日 09时36分41秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
/*!
* \file rkav_interface.h
* \brief rk3568 音视频控制接口
* 
* 
* \author yangkun
* \version 1.0
* \date 2022.3.26
*/
#ifndef _RKAV_INTERFACE_H_
#define _RKAV_INTERFACE_H_

#include <stdint.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


/**
 * @brief 1-1获取版本
 * @param[in/out] major  大版本
 * @param[in/out] minor  小版本
 * @param[in/out] patch  开发迭代版本
 */
void rkav_getver(unsigned char *major, unsigned char *minor, unsigned short *patch);


/**
 * @brief 出厂复位
 * @return 0 success, other failed
 */
int rkav_factory_reset();

/**
 * @brief 2-1设置视频输入，目前只有hdmi,不用设置
 * @param[in] sel  0 hdmi 1 usb
 * @return 0 success, other failed
 */
int rkav_vi_sel(int sel);

/**
 * @brief 2-1 设置音频输入
 * @param[in] sel 0 hdmi; 1 mic/line
 * @return 0 success, other failed
 */
int rkav_ai_sel(int sel);


/**
 * @breif 设置 mic/line 类型
 * @param[in] type 0 mic  1 line 
 */
int rkav_ai_set35_type(int type);

/**
 * @brief 2-3 设置音频输出通路
 * @param[in] sel 分辨率选择
 *			bit0: hdmi
 *			bit1: usb_out
 *			bit2: live
 *			bit3: record
 * return 0 succss, other failed
 */
int rkav_ao_sel(unsigned char sel);

/**
 * @brief 2-3 设置3.5耳机输出
 * @param[in] sel 分辨率选择
 *			bit0: hdmi
 *			bit1: mic_line
 *			bit2: usb_in
 *			bit3: playback
 *			bit7: mix
 * return 0 succss, other failed
 */
int rkav_ao_35_sel(unsigned char sel);

/**
 * @brief 2-3 设置hdmi输出分辨率帧率
 * @param[in] sel 分辨率选择
 * 				0 = None 不变
 *  			1 = 1920x1080p60
 *  			2 = 1920x1080p59.94
 *  			3 = 1920x1080p50
 *  			4 = 1920x1080p30
 *  			5 = 1920x1080p29.97
 *  			6 = 1920x1080p25
 *  			7 = 1920x1080p24
 *  			8 = 1920x1080p23.98
 *  			9 = 1280x720p60
 *  			10= 1280x720p59.94
 *  			11= 1280x720p50
 *
 * return 0 succss, other failed
 */
int rkav_hdmi_out_set(int sel);

/**
 * @brief 2-3 设置uvc_out输出分辨率帧率
 * @param[in] sel 分辨率选择
 * 				0 = None 不变
 *  			1 = 1920x1080p30
 *  			2 = 1920x1080p25
 *  			3 = 1280x720p60
 *  			4 = 1280x720p30
 *
 * return 0 succss, other failed
 */
int rkav_uvc_out_set(int sel);

/**
 * @brief 推流控制
 * @param[in] start 0 停止推流，忽略url
 *            start 1 开始推流， url要有效
 * @param[in] url 推流的url
 * return 0 success, other failed
 */
int rkav_push(int start, char *url);

/**
 * @brief 推流控制, cloud下发url, osee_live保存url
 * @param[in] start 0 停止推流
 *            start 1 开始推流
 * @param[in] url 推流的url
 * return 0 success, other failed
 */
int rkav_push_v1(int start);

/**
 * @brief 录像设置 
 * @param[in] start 0 停止录像，忽略name
 *            start 1 开始录像， name要有效
 * @param[in] url 推流的url
 * return 0 success, other failed
 */
int rkav_record(int start, char *name);

/**
 * @brief 视频编码类型设置,会停止录像推流等
 * @param[in] type 0 h264 1 h265
 * @return 0 success, other failed
 */
int rkav_set_video_encode_type(int type);

/**
 * @brief 视频编码码率设置
 * @param[in] bitrate 要设置的码率,单位dbps
 * @return 0 success, other failed
 */
int rkav_set_video_bitrate(int bitrate);

/**
 * @brief 视频编码码率设置
 * @param[in] bitrate 要设置的码率,单位dbps
 * @return 0 success, other failed
 */
int rkav_set_audio_bitrate(int bitrate);



/**
 * @brief 获取输入视频格式
 * return [0,12]  不同的分辨率格式
 */
int rkav_get_hdmi_in_type();

typedef struct s_rkav_in_type
{
	unsigned char hdmi_type; // hdmi输入信号类型
	unsigned char usb_type;  // usb 输入类型
	unsigned char play_type; // 本地播放文件类型
    unsigned char ndi_type; // 本地播放文件类型
}t_rkav_in_type;
/**
 * @brief 获取所有输入视频格式
 * return 0 success, other failed
 */
int rkav_get_in_type(t_rkav_in_type *type);

/**
 * @brief 获取推流网络状态
 * @return 0 正常，1 丢包
 */
int rkav_get_live_stats();

/**
 * @brief 获取当前视频编码码率
 * @return 0 6000kpbs; 1 4500 kbps; 2 3000kbps;
 */
int rkav_get_enc_bitrate();

/**
 * @brief 设置osd
 * @return 0 success, other failed
 */
int rkav_set_enable_osd(const char *png_path);

/**
 * @brief 设置osd, 根据显示分辨率自动挑选显示分辨率的逻辑
 * @parm png_path_1080p 1080p osd png 图片路径
 * @parm png_path_720p 720p osd png 图片路径
 * @return 0 success, other failed
 */
int rkav_set_enable_osd_v1(const char *png_path_1080p, const char *png_patp_720p);

/**
 * @brief 停止 osd
 * @return 0 success, other failed
 */
int rkav_set_disable_osd();

/**
 * @brief 打开mp4文件并进行播放
 * @param[in] path mp4文件路径
 * @return 0 success, other failed
 */
int rkav_mp4_open(char *path);

/**
 * @brief 关闭mp4文件播放
 * @return 0 success, other failed
 */
int rkav_mp4_close();

/**
 * @brief 对正在播放的mp4文件进行seek操作
 * @param[in] seek 偏移位置， 但是秒
 * @return 0 success, other failed
 */
int rkav_mp4_set_seek(int seek);

// 弃用次api
int rkav_mp4_get_seek(unsigned int *seek);

/**
 * @brief 获取正在播放的mp4文件seek
 * @param[out] seek 偏移位置， 但是秒
 * @param[out] duration 总时长秒
 * @return 0 success, other failed
 */
int rkav_mp4_get_seek_v1(unsigned int *seek, unsigned int *duration);

/**
 * @brief 设置是否暂停
 * @param[in] pause  0 不暂停  1 暂停
 * @return 0 success, otherfailed
 */
int rkav_mp4_set_pause(int pause);

/**
 * @brief 获取是否暂停
 * @return 0 success, 0 不暂停 1 暂停
 */
int rkav_mp4_get_pause();

/**
 * @brief 设置循环播放
 * @param[in]  loop 0 不循环播放 1 循环播放
 * @return 0 success, other failed
 */
int rkav_mp4_set_loop(int loop);

/**
 * @brief 获取循环播放 状态
 * @return  loop 0 不循环播放 1 循环播放
 */
int rkav_mp4_get_loop();

/**
 * @brief 获取播放状态
 * @return   0 不播放 1 播放
 */
int rkav_mp4_get_state();

/**
 * @brief 暂停时播放 下一帧，上一帧 ; 下x帧  上x帧
 * @param[in]  seq [-65535, 65536], 当前帧的前后范围， 目前仅支持1， 下一帧
 * @return 0 success, other failed
 */
int rkav_mp4_play_frame(int seq);


/**
 * @brief 设置MP4播放速度
 * @param[in] speed   -1 0.5倍速; 0 正常倍速； 1 2倍速；
 * @return 0 success, other failed
 */
int rkav_mp4_set_speed(int speed);

/**
 * @brief 获取MP4播放速度 
 * @param[in] speed   -1 0.5倍速; 0 正常倍速； 1 2倍速；
 * @return 0 success, other failed
 */
int rkav_mp4_get_speed(int *speed);



/**
 * @brief zooz，将mipi屏幕区域rect(x,y, w, h)区域放大的全屏
 * 			注意：mipi屏幕宽高为:1080x1920, x,y,w,h 要求2对齐
 *  @param[in] x 选择区域左上角坐标x
 *  @param[in] y 选择区域左上角坐标y
 *  @param[in] w 选择区域 宽度
 *  @param[in] h 选择区域 高度
 * @reurn 0 success, other failed
 */
int rkav_zoom_set(int x, int y, int w, int h);




/**
 * @brief rkav_audio_meter_get 获取音频表数据
 * @param[in] src 0 hdmi; 1 mic/line
 * @param[out] vu_leff  左声道 vu
 * @param[out] pk_leff  左声道 peak
 * @param[out] vu_right 右声道 vu
 * @param[out] pk_right 右声道 peak
 * return 0 success , other failed
 */
int rkav_audio_meter_get(int src, int *vu_left, int *pk_left, int *vu_right, int *pk_right);

typedef struct s_meter_one
{
	int vu_left;
	int vu_right;
	int pk_left;
	int pk_right;
}t_meter_one;

typedef struct s_meter
{
	t_meter_one line_mic; // line mic 输入 meter
	t_meter_one hdmi_in;  // hdmi 音频meter
	t_meter_one usb_in;   // usb camera 音频meter
	t_meter_one out;      // 输出音频meter
	t_meter_one sd;      // SD播放文件音频meter
}t_meter;

/**
 * @brief rkav_audio_meter_get_v1  获取音频表数据
 * @param[in] meter 音频表数据
 * @return 0 success, other failed
 */
int rkav_audio_meter_get_v1(t_meter *meter);



/**
 * @brief 设置输入输出音量
 * @ param[in] type 音量类型 01 hdmi_in  02 usb 04 mic/line 05 输出
 * @ param[in] volume 0 0db, 1 0.5db,  2 1db,  3 1.5db ..... 12 6db;
 *                          -1 -0.5db,-2 -1 db, -3 -1.5db .... -80 -40db
 * return 0 success, other failed
 */
int rkav_set_volume(int type, int volume);


/**
 * @brief qt 准备好后，调用此命令，osee_live对视频进行输出，防止出现osee_live 比qt先
 * 			先运行的情况
 */
void rkav_qt_run_ok();

/**
 * @brief 设置旋转类型
 * @param[in] type   e_osee_rotate_type
 * 					E_ROTATE_NORMAL 正常
 * 					E_ROTATE_MIRROR 镜像
 * 					E_ROTATE_INVERT 翻转
 * @return 0 success,other failed
 */
int rkav_set_rotate_type(int type);

/**
 * @brief 获取翻转类型
 * @return e_osee_rotate_type 
 */
int rkav_get_rotate_type();


/**
 * @brief 打开云服务
 */
int rkav_cloud_open();

/**
 * @brief 关闭云服务
 */
int rkav_cloud_close();


/**
 * @brief 获取three words 是的回调
 * @buf  three words 缓冲区
 * @len three words 长度
 * @return 0 success, other failed
 */
typedef int (*rkav_cb_three_words)(const char *buf, int len);

/**
 * @brief 连接回调
 * @is_connect 0 断开连接  1 成功连接
 * @return 0 success, other failed
 */
typedef int (*rkav_cb_connect)(int is_connect);

/**
 * @brief url 配置状态回调, 只有cloud设置url后，qt才能发送推流url
 * @is_config 0 未设置url  1 设置自定义url  2 youtube 3 facebook 4 twitch 5 Custom 6 frameio
 * @stream key  对应rtmp 推流的 stream key 如果是frameio时，stream_key代表 frameio 功能名
 * @account_name atomos 登录的账户名
 * @return 0 success, other failed
 */
typedef int (*rkav_cb_push_url_state)(int is_config, const char *stream_key, const char *account_name);

/**
 * @brief 设置 three word 回调
 * @return 0 success, other failed
 */
int rkav_set_three_words_cb(rkav_cb_three_words cb);

/**
 * @brief 设置 连接回调
 * @return 0 success, other failed
 */
int rkav_set_connect_cb(rkav_cb_connect cb);

/**
 * @brief 设置 url配置状态回调
 * @return 0 success, other failed
 */
int rkav_set_push_url_state_cb(rkav_cb_push_url_state cb);

/// @brief frameio 传输状态结构
typedef struct s_frameio_push_state
{
	/// 用于显示上传进度
	unsigned long long push_bytes; 	// 当前上传的文件已经上传的字节数 
	unsigned long long file_bytes; 	// 当前上传文件大小
	char *file_path;          // 当前正在上传的文件路径

	unsigned int file_queue_size;   // 上传队列总的文件数

}t_frameio_push_state;


/// @brief frameio 文件传输完成后，通知结构
typedef struct s_frameio_file_complete
{
	const char *file_path;          // 文件传输完成后，通知qt 此文件已经传递完成
}t_frameio_file_complete;

/**
 * @brief 获取frameio 上传文件的状态
 * @return 0 success, other failed
 */
int rkav_get_frameio_push_state(t_frameio_push_state *state);

/**
 * @brief 获取quality
 * @param[in] type cloud 类型:c2c rtmp
 * @paramp[in] resolution 当前编码输出分辨率
 * @param[in] fps        当前输出帧率
 */
void rkav_cloud_get_quality(unsigned char type, unsigned char resolution, unsigned char fps);

/// @brief 推流状态结构体
typedef struct s_push_state
{
	unsigned char state;  /**< 0 停止推流; 1 网络正常; 2 网络不好*/
	unsigned int bit_rate; /**< 推流网速 */
}t_push_state;
/**
 * @brief  获取推流网络状态
 * @param state  推流状态结构
 * @return 0 success, other failed
 */
int rkav_get_push_state(t_push_state *state);

/**
 * @brief 设置pip位置
 * @param[in] x pip 左上角坐标 x
 * @param[in] y pip 左上角坐标 y
 * @param[in] w pip 宽
 * @param[in] h pip 高
 * @return 0 success, other failed
 */
int rkav_pip_set_position(unsigned int x, unsigned int y, unsigned int w, unsigned int h);

/**
 * @brief 设置pip 源
 * @param[in] sel  -1 不使能(关闭)pip;  0 hdmi; 1 uvc camera; 2 sd card(mp4 play)
 * @return 0 success, other failed
 */
int rkav_pip_sel(int sel);


/**
 * @brief 设置pip 透明度
 * @param[in] opacity [0,255] default 0
 * @return 0 success, other failed
 */
int rkav_pip_set_opacity(unsigned char opacity);


/**
 * @brief 设置pip 与 overlay层级关系
 * @param[in] hierarchical
 *  // 1 pip在上 overlay 在下 默认
 *  // 2 overlay在上， pip在下
 *
 * @return 0 success, other failed
 */
int rkav_pip_set_hierarchical(int hierarchical);


void send_frameio_record(const char *path);

/******************************************* ndi ****************************************************************/
/**
 * @brief 搜索ndi设备
 * @param[in] timeout 搜索超时时间，单位ms
 * @return 0 success, other failed
 */
int rkav_ndi_search(int timeout);

/**
 * @brief 需要连接的ndi设备
 * @param[in] str ndi设备字符串
 * @param[in] len ndi字符串长度
 * @return 0 success, other failed
 */
int rkav_ndi_connect(char *str, int len);

/**
 * @brief 断开ndi设备
 */
void rkav_ndi_disconnect();
/******************************************* ndi ****************************************************************/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
#endif//_RKAV_INTERFACE_H_

