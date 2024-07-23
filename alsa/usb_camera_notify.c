/******************************************************************************
 *
 *       Filename:  usb_camera_notify.c
 *
 *    Description:  usb camera 插入拔下检测，
 *    主要原理：检测/dev/ 下是否增加 或者 减少了 videox文件
 *
 *        Version:  1.0
 *        Created:  2021年12月20日 17时12分59秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
/*!
* \file  usb_camera_notify.c
* \brief  检测/dev/ 下是否增加 或者 减少了 videox文件， 调用usb camera 获取或者取消线程
* 
* 
* \author yangkun
* \version 1.0
* \date 2021.12.20
*/
#include<stdio.h>
#include<assert.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/inotify.h>
#include<limits.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<sys/time.h>


#include <pthread.h>
#include <unistd.h>

#include "usb_camera_get.h"
#include "usb_camera_notify.h"

#define NOTIFY_PATH_LEN 100
typedef struct s_usb_camera_notify
{
	int inotifyFd;
	char path[NOTIFY_PATH_LEN]; // usb camera 路径
	unsigned int resolution; // 1 1920x1080 2 1280x720
	unsigned int fps;
	int cb_type;	// 1 插入  2 拔下
	usb_camera_notify_cb cb; // 插拔回调

	pthread_t th_usb_camera_notify; //  线程
	int quit;    // 0 不退出 1 退出

	long handle;
}t_usb_camera_notify;

static t_camera_info info;

static char *notify_path = "/dev/";
char path[100];

#define BUF_LEN 1000
static void check(struct inotify_event *i, t_usb_camera_notify *p_notify)
{
	printf("wd = %2d;", i->wd);
	int  ret;

	if(i-> cookie > 0)
	{
		printf("cookie = %4d;",i->cookie);
	}

	if(i->mask & IN_CREATE) printf("IN_CREATE\n");
	if(i->mask & IN_DELETE) printf("IN_DELETE\n");
	if(i->mask & IN_CREATE)
	{
		strcpy(path, notify_path);
		strcat(path, i->name);
		printf("create %s\n", i->name);

		// 检查 名字是否支持
		ret = check_camera_path(path, &info);
		printf("check camera path ret=%d\n", ret);
		// 是否插入
		if(p_notify->cb && ret == 0)
		{
			strcpy(p_notify->path, path);
			p_notify->resolution = info.resolution;
			p_notify->fps = info.frame_rate;
			p_notify->cb(p_notify->path, 1, p_notify->resolution, p_notify->fps, p_notify->handle);
		}
	}

	if(i->mask & IN_DELETE)
	{
		strcpy(path, notify_path);
		strcat(path, i->name);
		printf("delete %s\n", i->name);

		// 检查是否已经打开, 如果打开则关机
		if(0 == memcmp(path, p_notify->path, strlen(path)))
		{

			if(p_notify->cb)
			{
				p_notify->cb(p_notify->path, 2, p_notify->resolution, p_notify->fps, p_notify->handle);
			}

		}

	}
	
}
static void *usb_camera_notify_proc(void *param)
{
	if(param == NULL)
	{
		printf("ERROR in get notify param\n");
		exit(1);
	}
	t_usb_camera_notify *p_notify = (t_usb_camera_notify *)(param);
	
    int inotifyFd, wd, j;
    char buf[BUF_LEN];
    char *p;
    struct inotify_event *event;
    int flages;

    ssize_t numRead;
    fd_set inputs, testfds;
    struct timeval timeout;
    int result;


    inotifyFd = inotify_init(); //系统调用可创建一新的inotify实例  初始化实例
    if(inotifyFd == -1)
    {
        printf("error in inotify init\n");
		exit(1);
    }

    wd = inotify_add_watch(inotifyFd, notify_path, IN_CREATE|IN_DELETE);  //系统调用inotify_add_watch()可以追加新的监控项
    if(wd == -1)
    {
        printf("error int wd\n");
		exit(1);
    }
    printf("watching %s using wd =  %d\n", notify_path, wd);

	p_notify->inotifyFd = inotifyFd;

    while(!p_notify->quit)  //开启非阻塞select模式  
    {
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;//500000;
		//printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>================================>\n");
        int fd;
		FD_ZERO(&inputs);
		FD_SET(inotifyFd, &inputs);  //将inotify的文件秒速符加入到 select中
        testfds = inputs;
        result = select(FD_SETSIZE, &testfds, (fd_set *)0, (fd_set *)0, (struct timeval *) &timeout);
		if(result == 0)
		{
			// timeout
			continue;
		}
        if (result < 0)
        {
            perror("server err");
			//exit(1);
			break;
        }
        for(fd = 0; fd < FD_SETSIZE; fd++)
        {
            if(FD_ISSET(fd, &testfds))   //收到消息 处理消息
            {
                if(fd == inotifyFd)
                {	
					//sleep(2);
					usleep(500*1000);
					//printf("==============================\n");
                            numRead = read(inotifyFd,buf,BUF_LEN);
                             if(numRead == -1)
                            {
                                printf("read error\n");
                              }

                             printf("read %ld bytes from inotify fd\n",(long)numRead);
                             for(p = buf; p < buf+numRead;)
                             {
                               event = (struct inotify_event *)p;
                              check(event, p_notify);
                              p+=sizeof(struct inotify_event)+ event->len;
                             }//for
                   }//if()
            }//if(fd)
        }//for()
    }//while(1)
	if(p_notify->inotifyFd)
	{
		close(p_notify->inotifyFd);
		p_notify->inotifyFd = -1;
	}


	return NULL;
}

int init_usb_camera_notify_v1(NHANDLE *hd, usb_camera_notify_cb _cb, long handle, char *path)
{
	int len;
	//  创建线程监听 /dev/ 目录
	int ret;
	// 分配内存
	t_usb_camera_notify *p_notify = (t_usb_camera_notify *)malloc(sizeof(t_usb_camera_notify));
	if(p_notify == NULL)
	{
		printf("ERROR in malloc usb camera notify memory\n");
		return -1;
	}
	memset(p_notify, 0, sizeof(t_usb_camera_notify));

	p_notify->handle = handle;

	if(path!= NULL)
	{
		len = strlen(path);	
		// 过长检测
		if(len >= NOTIFY_PATH_LEN)
		{
			printf("ERROR in notify path, too len:%d\n", len);
			exit(1);
		}
		strcpy(p_notify->path, path);
	}

	p_notify->cb = _cb;


	// 创建线程
	p_notify->quit = 0;
	ret = pthread_create(&p_notify->th_usb_camera_notify, NULL, usb_camera_notify_proc, p_notify);
	if(ret != 0)
	{
		printf("ERROR to create usb camera notify thread:%s\n", strerror(errno));
		exit(1);
	}

	*hd = (NHANDLE)p_notify;

	return 0;
error:
	free(p_notify);
	return ret;
}



int init_usb_camera_notify(NHANDLE *hd, usb_camera_notify_cb _cb)
{
	return init_usb_camera_notify_v1(hd, _cb, 0, NULL);
}

int deinit_usb_camera_notify(NHANDLE *hd)
{
	if(*hd == NULL)
	{
		printf("ERROR in get notify param\n");
		return 0;
	}
	t_usb_camera_notify *p_notify = (t_usb_camera_notify *)(*hd);
	if(p_notify->inotifyFd)
	{
		close(p_notify->inotifyFd);
		p_notify->inotifyFd = -1;
	}
	printf("close p_notify->inotifyFd\n");
	// 停止监听线程
	p_notify->quit = 1;
	pthread_join(p_notify->th_usb_camera_notify, NULL);

	free(p_notify);
	*hd = NULL;

	return 0;
}
