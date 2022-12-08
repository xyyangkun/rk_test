/******************************************************************************
 *
 *       Filename:  test.c
 *
 *    Description: test
 *
 *        Version:  1.0
 *        Created:  2022年10月08日 18时14分12秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <stdio.h>
#include <signal.h>
#include "h264_dec.h"

#include <iostream>
#include <memory>
#include "easymedia/rkmedia_api.h"
#include "easymedia/buffer.h"
#include "h264_rkmedia_dec.h"

// 使用rkmedia解码, 注释后直接使用mpp库解码
// #define USE_RKMEDIA_DEC

std::shared_ptr<easymedia::ImageBuffer> ib;
static bool quit = false;
static void sigterm_handler(int sig) {
	quit = true;
}

// 单纯的测试解码
int test_dec()
{

#ifndef USE_RKMEDIA_DEC
	// mpp 解码
	start_test_h264_dec();
#else
	// rkmedia 解码
	start_test_h264_rkmedia_dec();
#endif

	while(!quit) {
		usleep(100*1000);
	}
	printf("==========================> will exit!\n");
#ifndef USE_RKMEDIA_DEC
	stop_test_h264_dec();
#else
	stop_test_h264_rkmedia_dec();
#endif


	return 0;
}

// 测试多次解码，解码后释放解码器，再次创建解码
int test_decs()
{
	// 运行次数
	int times = 2;

	for(int i=0; i<times; i++)
	{
		quit = false;;
#ifndef USE_RKMEDIA_DEC
		// mpp 解码
		start_test_h264_dec();
#else
		// rkmedia 解码
		start_test_h264_rkmedia_dec();
#endif

		// 间隔延迟
		while(!quit) {
			usleep(100*1000);
		}

		printf("==========================> will exit!\n");
#ifndef USE_RKMEDIA_DEC
		stop_test_h264_dec();
#else
		stop_test_h264_rkmedia_dec();
#endif
	}

	return 0;

}

int main()
{
	signal(SIGINT, sigterm_handler);
	signal(SIGTERM, sigterm_handler);

	int ch = 0;
	if (ch == 0)
	{
		test_dec();
	}

	if (ch == 1)
	{
		test_decs();
	}

}
