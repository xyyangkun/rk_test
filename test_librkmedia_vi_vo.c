/******************************************************************************
 *
 *       Filename:  test_librkmedia_vi_vo.c
 *
 *    Description: test librkmedia_vi_vo
 *
 *        Version:  1.0
 *        Created:  2022年10月31日 13时52分22秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "librkmedia_vi_vo.h"

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}


int main()
{
	int ret;

	ret = librkmedia_vi_vo_init();
	assert(ret == 0);

	printf("%s initial finish\n", __func__);
	signal(SIGINT, sigterm_handler);
	while (!quit) {
		usleep(500000);
	}

	ret = librkmedia_vi_vo_deinit();
	assert(ret == 0);

	printf("%s exit!\n", __func__);

}
