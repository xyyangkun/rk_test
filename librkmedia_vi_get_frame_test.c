/******************************************************************************
 *
 *       Filename:  librkmedia_vi_get_frame_test.c
 *
 *    Description:  test librkmedia_vi_get_frame
 *
 *        Version:  1.0
 *        Created:  2022年11月10日 14时51分44秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include "librkmedia_vi_get_frame.h"
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

static bool quit = false;
static void sigterm_handler(int sig) {
	fprintf(stderr, "signal %d\n", sig);
	quit = true;
}

void YuvSave(void *buf, unsigned int size)
{
#if 0
	static int index_raw = 0;
	char nv12_path[64];
	sprintf(nv12_path, "test_nv12_%d.yuv", index_raw++);
	FILE *file = fopen(nv12_path, "w");
	if (file) {
		fwrite(buf, 1, size, file); 
		fclose(file);
	}
#endif
	printf("get buf size:%d\n", size);
}


int main()
{
	int ret;
	signal(SIGINT, sigterm_handler);

	// 初始化
	ret = init_vi(&YuvSave);
	if(ret != 0)
	{
		printf("error in init!\n");
		exit(1);
	}

	while (!quit) {
		usleep(500000);
	}

	// 释放
	deinit_vi();
}
