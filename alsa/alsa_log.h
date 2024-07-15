//
// Created by win10 on 2024/6/13.
//

#ifndef MP4_DIRECT_WRITER_MP4_LOG_H
#define MP4_DIRECT_WRITER_MP4_LOG_H
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

// 定义日志回调函数类型
typedef void (*log_callback_t)(const char *message);
void set_error_log_callback(log_callback_t callback);
void set_info_log_callback(log_callback_t callback);
void set_debug_log_callback(log_callback_t callback);
void error_log_message(const char *format, ...);
void info_log_message(const char *format, ...);
void debug_log_message(const char *format, ...);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
#endif //MP4_DIRECT_WRITER_MP4_LOG_H
