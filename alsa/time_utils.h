//
// Created by win10 on 2024/2/20.
//

#ifndef RV1106_MEDIA_TIME_UTILS_H
#define RV1106_MEDIA_TIME_UTILS_H
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
// 输出时间差的文件
#define TIME_DIFF_OUTPATH "/tmp/audio_time_diff"
// 创建时间handle
void *time_diff_create(const char *time_diff_name);
void time_diff_delete(void **hd);
// 开始计算时间
void time_diff_start(void *hd);
// 计算从开始时间到现在时间差
void time_diff_end(void *hd);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif //RV1106_MEDIA_TIME_UTILS_H
