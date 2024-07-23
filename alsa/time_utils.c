//
// Created by win10 on 2024/2/20.
//

#include <stdint.h>
#include <stdbool.h>
#include <memory.h>
#include <malloc.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "time_utils.h"

#include <inttypes.h> // PRIu64



typedef struct s_time_diff{
    bool start;
    uint64_t time_start; // 开始时间，计算平均时间使用
    uint64_t time_last;  // 上一次的时间
    uint64_t time_end;   // 结束时间，也是本次时间

    uint64_t time_last_print;  // 上一次输出的时间

    uint64_t time_diff_max;    // 记录时间差最大值
    uint64_t time_diff_min;  // 记录时间差最小值
    uint64_t time_all;
    uint64_t time_diff_average;        // 记录时间差
    uint64_t time_diff;        // 记录时间差
    uint64_t time_count;  // 启示录次数

    uint64_t time_60_count;  // 超过%60时间的次数
    FILE *fp;
    char buff[1024];
}t_time_diff;

#define GET_CURRENT_TMP struct timespec system_time;

#define GET_CURRENT_MS() \
    clock_gettime(CLOCK_MONOTONIC, &system_time), system_time.tv_sec*1000 +  system_time.tv_nsec/1000


// 创建时间handle
void *time_diff_create(const char *time_diff_name){
    t_time_diff * diff = (t_time_diff *)malloc(sizeof(t_time_diff));
    memset(diff, 0, sizeof(t_time_diff));
    diff->start = false;
    diff->time_count = 0;
    diff->time_all = 0;
    diff->fp = fopen(time_diff_name, "wb");
    if(!diff->fp){
        printf("ERROR in time_diff_create:%s\n", strerror(errno));
        exit(1);
    }
    setbuf(diff->fp, NULL);
    return diff;
}

void time_diff_delete(void **hd){
    t_time_diff * diff = (t_time_diff * )*hd;
    fclose(diff->fp);
    free(diff);
    diff = NULL;
}

// 开始计算时间
void time_diff_start(void *hd){
    struct timespec system_time;
    t_time_diff * diff = (t_time_diff * )hd;
    clock_gettime(CLOCK_MONOTONIC, &system_time);
    if(diff->start == false){
        diff->start = true;
        diff->time_start = system_time.tv_sec*1000000ULL +  system_time.tv_nsec/1000ULL;
        diff->time_last_print = diff->start;
    }
    diff->time_last = system_time.tv_sec*1000000ULL +  system_time.tv_nsec/1000ULL;
}

// 计算从开始时间到现在时间差
void time_diff_end(void *hd) {
    struct timespec system_time;

    t_time_diff * diff = (t_time_diff * )hd;
    diff->time_count ++;
    //if(diff->time_count < 2 )return ; // 刚开始可能比较大，过滤掉
    clock_gettime(CLOCK_MONOTONIC, &system_time);
    diff->time_end = system_time.tv_sec*1000000ULL +  system_time.tv_nsec/1000ULL;
    diff->time_diff = diff->time_end - diff->time_last;
    diff->time_all += diff->time_diff;
    // printf("end:%llu  %llu   %llu\n", diff->time_end, diff->time_last, diff->time_diff );

    if(diff->time_diff_max == 0)diff->time_diff_max = diff->time_diff;
    if(diff->time_diff_min == 0)diff->time_diff_min = diff->time_diff;

    if(diff->time_diff < diff->time_diff_min){
        diff->time_diff_min = diff->time_diff;
    }
    if(diff->time_diff > diff->time_diff_max){
        diff->time_diff_max = diff->time_diff;
    }
    // diff->time_diff_average = (diff->time_end - diff->time_start)/diff->time_count;
    diff->time_diff_average = (double)diff->time_all/(double)diff->time_count;

    if(diff->time_diff >= 6000){
        diff->time_60_count ++;
    }

    if(diff->time_end - diff->time_last_print > 1000) {
        // 输出打印信息,最大时间， 平均时间， 最小时间
        //printf("ddd=>\n");
        sprintf(diff->buff, "%s %" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t% " PRIu64 "\t% " PRIu64 "\n",
                "max\taverage\tmin\tcurrent\t%60count\tallcount  (us)\n", diff->time_diff_max, diff->time_diff_average, diff->time_diff_min, diff->time_diff,
                diff->time_60_count, diff->time_count);
        //printf("==>%s\n", diff->buff);
        fseek(diff->fp, 0, SEEK_SET);
        //truncate(TIME_DIFF_OUTPATH, 0);

        //printf("size:%d\n", strlen(diff->buff));
        fwrite(diff->buff, 1, strlen(diff->buff)+1, diff->fp);
        fflush(diff->fp);
        diff->time_last_print = diff->time_end;
    }
}
