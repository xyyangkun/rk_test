/******************************************************************************
 *
 *       Filename:  test_sd_event.c
 *
 *    Description:  测试sd卡 插拔事件
 *    // from qthread_uevent.cpp
 *
 *        Version:  1.0
 *        Created:  2022年10月12日 15时38分00秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <stdbool.h>
#include <unistd.h>

#include "uevent.h"

bool g_running;



static void sigterm_handler(int sig) {
	g_running = false;
}

int main()
{
	g_running = true;

	signal(SIGINT, sigterm_handler);

	int ret;

	start_uevent();

	while(g_running == true)
	{
		usleep(1000*1000);
		printf("g_have_sd=%d\n", g_have_sd);
	}

	stop_uevent();

	printf("=============================>0 stop!!\n");

}
