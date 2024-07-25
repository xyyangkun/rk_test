//
// Created by xy on 2024/2/17.
// 特效头文件
//

#ifndef OSEE_EFFECTS_H
#define OSEE_EFFECTS_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


/// 一次处理pcm的点数
#define OSEE_PROC_SIZE 480
//#define OSEE_PROC_SIZE 384
//#define OSEE_PROC_SIZE 192
//#define OSEE_PROC_SIZE 480
//#define OSEE_PROC_SIZE (768/2/2)
#define OSEE_SAMPLE_RATE 48000

int osee_reverb_init();

int osee_reverb_deinit();

/**
 * @brief 设置reverb 参数
 * @param[in]  audio_chn    [0, 1]  左右声道
 * @param[in] reverberance  [0,  100]
 * @param[in] hf_damping    [0,  100]
 * @param[in] room_scale    [0,  100]
 * @param[in] stereo_depth  [0,  100]
 * @param[in] pre_delay_ms  [0,  500]
 * @param[in] wet_gain_dB   [-10, 10]
 * @return 0 succes, others error
 */
int osee_reverb_set(int audio_chn, char*  reverberance, char*  hf_damping, char*  room_scale, char*  stereo_depth,
                    char*  pre_delay_ms, char*  wet_gain_dB);

/**
 * @brief 处理reverb数据
 * @return 0 succes, others error
 */
int osee_reverb_flow0(void *in, void *out);
int osee_reverb_flow1(void *in, void *out);


int osee_equalizer_init();
int osee_equalizer_deinit();
/**
 * @brief 设置reverb 参数
 * @param[in]  num          [0, 5]  第几路
 * @param[in]  audio_chn    [0, 1]  左右声道
 * @param[in] frequency
 * @param[in] width
 * @param[in] gain
 * @return 0 succes, others error
 */
int osee_equalizer_set(int num, int audio_chn, char *frequency, char *width, char *gain);

/**
 * @brief 处理equalizer数据
 * @return 0 succes, others error
 */
int osee_equalizer_flow0(void *in, void *out);
int osee_equalizer_flow1(void *in, void *out);



int osee_ns_init();
int osee_ns_deinit();

int osee_ns_set(int enable);

int osee_ns_flow0(void *in, void *out, int level);
int osee_ns_flow1(void *in, void *out, int level);


int osee_compand_init();
int osee_compand_deinit();
/**
 * @brief 设置 compand 参数
 * @param[in]  num          [0, 5]  第几路
 * @param[in]  audio_chn    [0, 1]  左右声道
 * @param[in] frequency
 * @param[in] width
 * @param[in] gain
 * @return 0 succes, others error
 */
int osee_compand_set(int num, int audio_chn, char *frequency, char *width, char *gain);

/**
 * @brief 处理equalizer数据
 * @return 0 succes, others error
 */
int osee_compand_flow0(void *in, void *out);
int osee_compand_flow1(void *in, void *out);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif //OSEE_EFFECTS_H
