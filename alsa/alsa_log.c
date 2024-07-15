//
// Created by win10 on 2024/6/13.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "alsa_log.h"



// 全局变量，存储当前的日志回调函数
static log_callback_t  error_log_callback = NULL;
static log_callback_t  info_log_callback  = NULL;
static log_callback_t  debug_log_callback = NULL;


// 设置日志回调函数
void set_error_log_callback(log_callback_t callback) {
    error_log_callback = callback;
}

// 设置日志回调函数
void set_info_log_callback(log_callback_t callback) {
    info_log_callback = callback;
}

// 设置日志回调函数
void set_debug_log_callback(log_callback_t callback) {
    debug_log_callback = callback;
}

// 记录日志信息的函数
void error_log_message(const char *format, ...) {
    if (error_log_callback != NULL) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        error_log_callback(buffer);
    } else {
        // 默认处理方式
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n"); // 结尾增加换行
    }
}

void info_log_message(const char *format, ...) {
    if (info_log_callback != NULL) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        info_log_callback(buffer);
    } else {
        // 默认处理方式
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n"); // 结尾增加换行
    }
}

void debug_log_message(const char *format, ...) {
    if (debug_log_callback != NULL) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        debug_log_callback(buffer);
    } else {
        // 默认处理方式
        va_list args;
        va_start(args, format);
        vfprintf(stdout, format, args);
        va_end(args);
        fprintf(stdout, "\n"); // 结尾增加换行
    }
}

#ifdef MP4_LOG_DEBUG
// 示例日志回调函数
void my_log_callback(const char *message) {
    printf("Custom Log: %s\n", message);
}

int test_mp4_log() {
    // 设置自定义日志回调函数
    set_debug_log_callback(my_log_callback);

    // 记录日志
    debug_log_message("This is a test log message: %d", 42);

    // 取消自定义日志回调函数，使用默认处理方式
    set_debug_log_callback(NULL);

    // 记录日志
    debug_log_message("This is another test log message: %s", "Hello, World!");

    return 0;
}
#endif