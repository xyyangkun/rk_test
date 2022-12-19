/******************************************************************************
 *
 *       Filename:  myutils.h
 *
 *    Description:  工具
 *
 *        Version:  1.0
 *        Created:  2021年09月01日 13时25分27秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef _MYUTILS_H_
#define _MYUTILS_H_

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#if 1
#define print_fps(name) do {\
	static int count=0; \
	static struct timeval t_old = {0}; \
	if(count%120 == 0) \
	{ \
		static struct timeval t_new; \
		gettimeofday(&t_new, 0); \
		printf("=====>%s:%ld\n",name,120*1000000/((t_new.tv_sec-t_old.tv_sec)*1000000 + (t_new.tv_usec - t_old.tv_usec)));\
		t_old = t_new; \
	} \
	count++; \
} while(0)
#else
#define print_fps(name)
#endif

#ifndef ALIGN16
#define ALIGN16(value) ((value + 15) & (~15))
#endif

#define DEBUG
#ifdef DEBUG
static int _dbg=0;

/// \brief 初始化是否要运行测试
/// \param void
/// \return void
static void _dbg_init()
{
#ifdef MODULE 
	if (getenv("dbg") || getenv(MODULE))
#else
	if (getenv("dbg"))
#endif
    {
        _dbg=1;
    }
}
#define dbg(fmt, args...)\
    do {\
        if (_dbg){printf("[%s %d]"fmt, __FUNCTION__, __LINE__, ##args);}\
	}\
    while(0)
#else
    #define dbg(fmt, args...)
    #define _dbg_init()
#endif

#endif//_MYUTILS_H_
