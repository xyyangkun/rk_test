/******************************************************************************
 *
 *       Filename:  uevent.c
 *
 *    Description: test event
 *
 *        Version:  1.0
 *        Created:  2022年10月12日 16时46分09秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <fcntl.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <sys/prctl.h>  /* asked by PR_SET_NAME */
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <dirent.h>

#include "uevent.h"


#define UEVENT_SUBSYSTEM_ANDROID		"android_usb"
#define UEVENT_SUBSYSTEM_BLOCK          "block"
#define UEVENT_SUBSYSTEM_MMC            "mmc"
#define UEVENT_SUBSYSTEM_NET            "net"

bool uevent_running;

bool g_have_sd;

void handle_block_event(struct uevent *uevent)
{
    if (strcmp(uevent->subsystem, UEVENT_SUBSYSTEM_BLOCK))
        return;

    if ((strcmp(uevent->device_name, "mmcblk1") == 0)
        && (strcmp(uevent->action, "remove") == 0))
    {
		printf("sd remove!\n");
		g_have_sd = false;
    }
}

void handle_mmc_event(struct uevent *uevent)
{
    if (strcmp(uevent->subsystem, UEVENT_SUBSYSTEM_MMC))
        return;

    if (strcmp(uevent->action, "bind") == 0)
    {
		printf("sd insert!\n");
		g_have_sd = true;
    }
}

void handle_net_event(struct uevent *uevent)
{
    if (strcmp(uevent->subsystem, UEVENT_SUBSYSTEM_NET))
        return;
	return ;
#if 0
    if (strcmp(uevent->action, "add") == 0)
    {
        if (strncmp(uevent->path, "/devices/platform/fd880000.usb", 30) == 0)
        {
            char *netif = get_filename(uevent->path);

            send_net_change(true, netif, "usb_in");
        }
        else if (strncmp(uevent->path, "/devices/platform/usbdrd/fcc00000.dwc3", 38) == 0)
        {
            char *netif = get_filename(uevent->path);

            send_net_change(true, netif, "usb_out");
        }
        else
        {
            char *netif = get_filename(uevent->path);

            send_net_change(true, netif, NULL);
        }
    }
    else if (strcmp(uevent->action, "remove") == 0)
    {
        if (strncmp(uevent->path, "/devices/platform/fd880000.usb", 30) == 0)
        {
            char *netif = get_filename(uevent->path);

            send_net_change(false, netif, "usb_in");
        }
        else if (strncmp(uevent->path, "/devices/platform/usbdrd/fcc00000.dwc3", 38) == 0)
        {
            char *netif = get_filename(uevent->path);

            send_net_change(false, netif, "usb_out");
        }
        else
        {
            char *netif = get_filename(uevent->path);

            send_net_change(false, netif, NULL);
        }
    }
#endif
}

static void _is_online()
{
#define SYS_PATH_SD "/sys/bus/mmc/devices"

	DIR *dp;
	struct dirent *drip;

	g_have_sd = false;

	if (access(SYS_PATH_SD, R_OK) != 0)
	{
		printf( "%s: access %s ERROR(%d-%s) !\n", __func__, SYS_PATH_SD, errno, strerror(errno));

		return;
	}

	if ((dp = opendir(SYS_PATH_SD)) == NULL)
	{
		printf( "%s: opendir %s ERROR(%d-%s) !\n", __func__, SYS_PATH_SD, errno, strerror(errno));

		return ;
	}

	while ((drip = readdir(dp)) != NULL)
	{
		if (strcmp(drip->d_name, ".") == 0 ||
				strcmp(drip->d_name, "..") == 0)
			continue;

		if (strncmp(drip->d_name, "mmc1:", 5) == 0)
		{
			g_have_sd = true;
			printf("=================> have sd!!\n");

			closedir(dp);

			return;
		}
	}

	closedir(dp);
}


void parse_event(const char *msg, int len, struct uevent *uevent)
{
    if (msg == NULL
		|| len <= 0)
		return;
	
	uevent->action = "";
	uevent->path = "";
	uevent->subsystem = "";
	uevent->usb_state = "";
	uevent->device_name = "";

	while(*msg) 
	{
		if(!strncmp(msg, "ACTION=", 7)) {
			msg += 7;
			uevent->action = msg;
		} else if(!strncmp(msg, "DEVPATH=", 8)) {
			msg += 8;
			uevent->path = msg;
		} else if(!strncmp(msg, "SUBSYSTEM=", 10)) {
			msg += 10;
			uevent->subsystem = msg;
		} else if(!strncmp(msg, "USB_STATE=", 10)) {
			msg += 10;
			uevent->usb_state = msg;
		} else if(!strncmp(msg, "DEVNAME=", 8)) {
			msg += 8;
			uevent->device_name = msg;
		}
		/* advance to after the next \0 */
		while(*msg++)
			;
	}

	if (uevent
		&& uevent->subsystem
		&& strlen(uevent->subsystem) > 0)
	{
#if 0
        printf("event { '%s', '%s', '%s', '%s', '%s' }\n",
                uevent->action, uevent->path, uevent->subsystem, uevent->usb_state, uevent->device_name);
#endif

        if (strcmp(uevent->subsystem, UEVENT_SUBSYSTEM_BLOCK) == 0)
        {
            handle_block_event(uevent);
        }
        else if (strcmp(uevent->subsystem, UEVENT_SUBSYSTEM_MMC) == 0)
        {
            handle_mmc_event(uevent);
        }
        else if (strcmp(uevent->subsystem, UEVENT_SUBSYSTEM_NET) == 0)
        {
            handle_net_event(uevent);
        }
	}
}



void *th_event(void *param)
{
	int sockfd;
    int len;
	char buf[1024 + 2];
	struct iovec iov;
	struct msghdr msg;
	struct sockaddr_nl sa;
    struct uevent uevent;

    prctl(PR_SET_NAME, "ThreadUevent", 0, 0, 0);

    sockfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
    if (sockfd == -1) {
        printf("socket creating failed:%s\n", strerror(errno));
        goto err_event_monitor;
    }

	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = NETLINK_KOBJECT_UEVENT;
	sa.nl_pid = 0;

    if (bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
        printf("bind error:%s\n", strerror(errno));
        goto err_event_monitor;
    }

    printf("NETLINK_KOBJECT_UEVENT create fd(%d)\n", sockfd);

	memset(&msg, 0, sizeof(msg));
	iov.iov_base = (void *)buf;
	iov.iov_len = sizeof(buf);
	msg.msg_name = (void *)&sa;
	msg.msg_namelen = sizeof(sa);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	while (uevent_running) {
		len = recvmsg(sockfd, &msg, 0);
		if (len < 0) {
            printf("receive error\n");
        } else if (len < 32 || len >= sizeof(buf)) {
            printf("invalid message");
		} else {
			buf[len] = '\0';
			buf[len + 1] = '\0';
			parse_event(buf, len + 2, &uevent);
		}
	}

err_event_monitor:
    printf("###################ThreadUevent %s EXIT ###############\n", __func__);
	pthread_detach(pthread_self());
	pthread_exit(NULL);

}

int start_uevent()
{
	int ret;
	pthread_t tid;

	_is_online();
	
	uevent_running = true;
	ret = pthread_create(&tid, NULL, th_event, NULL);
	if(ret != 0)
	{
		printf("ERROR to create thread!\n");
		exit(0);
	}
}


int stop_uevent()
{
	uevent_running = false;
}
