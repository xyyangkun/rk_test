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


static bool quit = false;
static void sigterm_handler(int sig) {
	quit = true;
}
int main()
{
	signal(SIGINT, sigterm_handler);

	start_test_h264_dec();

	while(!quit) {
		usleep(100*1000);
	}
	stop_test_h264_dec();
}
