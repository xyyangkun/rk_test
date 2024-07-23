/******************************************************************************
 *
 *       Filename:  usb_camera_get.c
 *
 *    Description:  usb camera info get 
 *
 *        Version:  1.0
 *        Created:  2021年12月20日 16时02分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
/*!
* \file usb_camera_get.c
* \brief usb_camera info get
*    	通过camera名称 /dev/video9 /dev/video10 获取支持jpg 最大支持1080p 720p分辨率的camera
*    	返回camera名字和最大支持分辨率
* 
* 
* \author yangkun
* \version 1.0
* \date 2021.12.20
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <linux/videodev2.h>
#include<sys/ioctl.h>
#include <setjmp.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "usb_camera_get.h"

int check_camera_path(char *path, t_camera_info *info)
{
	assert(path);
	int ret = 0;
	int cam_fd;
	if((cam_fd = open(path, O_RDWR)) == -1)
	{
		printf("ERROR in open %s  ==>%s\n", path, strerror(errno));
		return -1;
	}

	struct v4l2_capability cam_cap;
	if(ioctl(cam_fd,VIDIOC_QUERYCAP,&cam_cap) == -1)
	{
		perror("Error opening device %s: unable to query device.");
		ret = -2;
		goto end;
	}
	if((cam_cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) 
	{
		perror("ERROR video capture not supported.");
		ret = -3;
		goto end;
	}

	struct v4l2_format v4l2_fmt;
	memset(&v4l2_fmt, 0, sizeof(struct v4l2_format));
	v4l2_fmt.type = V4L2_CAP_VIDEO_CAPTURE;
	// 尝试设置1920x1080 jpg
#if 1
	v4l2_fmt.fmt.pix.width = 1920;
	v4l2_fmt.fmt.pix.height = 1080;
	v4l2_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	if (ioctl (cam_fd, VIDIOC_S_FMT, &v4l2_fmt) == -1) 
	{   
		perror("ERROR camera VIDIOC_S_FMT Failed.");
		close(cam_fd);
		return -1;
	}
#endif

#if 1
	struct v4l2_streamparm v4l2_parm;
	memset(&v4l2_parm, 0, sizeof(struct v4l2_streamparm));
	v4l2_parm.type = V4L2_CAP_VIDEO_CAPTURE;
	if (ioctl (cam_fd, VIDIOC_G_PARM, &v4l2_parm) == -1) 
	{
		perror("ERROR camera VIDIOC_G_FMT Failed.");
		//ret = -4;
		//goto end;
	}
	printf("frame inter: %d %d\n",
			v4l2_parm.parm.capture.timeperframe.numerator,
			v4l2_parm.parm.capture.timeperframe.denominator);
	if(v4l2_parm.parm.capture.timeperframe.denominator < 25)
		info->frame_rate = 30;
	else 
	{
		// 两数相除为帧率
		info->frame_rate = v4l2_parm.parm.capture.timeperframe.denominator/
			v4l2_parm.parm.capture.timeperframe.numerator;
	}

#endif


	if (ioctl (cam_fd, VIDIOC_G_FMT, &v4l2_fmt) == -1) 
	{
		perror("ERROR camera VIDIOC_G_FMT Failed.");
		ret = -4;
		goto end;
	}

#if 1
	printf("V4L2_PIX_FMT_MJPEG=%#x\n", V4L2_PIX_FMT_MJPEG);
	printf("get pixelformat :%#x width:%d height:%d, type=%#x\n",
			v4l2_fmt.fmt.pix.pixelformat,
			v4l2_fmt.fmt.pix.width,
			v4l2_fmt.fmt.pix.height,
			v4l2_fmt.type);
#endif
	if(v4l2_fmt.fmt.pix.pixelformat==V4L2_PIX_FMT_MJPEG &&
			v4l2_fmt.fmt.pix.width == 1920 &&
			v4l2_fmt.fmt.pix.height == 1080)
	{
		printf("success get camera info!!\n");
		strcpy(info->path, path);
		info->resolution = 1;
		goto end;
	}

	// 尝试1280x720 jpg
	memset(&v4l2_fmt, 0, sizeof(struct v4l2_format));
	v4l2_fmt.type = V4L2_CAP_VIDEO_CAPTURE;
#if 1
	v4l2_fmt.fmt.pix.width = 1280;
	v4l2_fmt.fmt.pix.height = 720;
	v4l2_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	if (ioctl (cam_fd, VIDIOC_S_FMT, &v4l2_fmt) == -1) 
	{   
		perror("ERROR camera VIDIOC_S_FMT Failed.");
		close(cam_fd);
		return -1;
	}
#endif

	if (ioctl (cam_fd, VIDIOC_G_FMT, &v4l2_fmt) == -1) 
	{
		perror("ERROR camera VIDIOC_G_FMT Failed.");
		ret = -4;
		goto end;
	}

#if 1
	printf("V4L2_PIX_FMT_MJPEG=%#x\n", V4L2_PIX_FMT_MJPEG);
	printf("get pixelformat :%#x width:%d height:%d, type=%#x\n",
			v4l2_fmt.fmt.pix.pixelformat,
			v4l2_fmt.fmt.pix.width,
			v4l2_fmt.fmt.pix.height,
			v4l2_fmt.type);
#endif
	if(v4l2_fmt.fmt.pix.pixelformat==V4L2_PIX_FMT_MJPEG &&
			v4l2_fmt.fmt.pix.width == 1280 &&
			v4l2_fmt.fmt.pix.height == 720)
	{
		printf("success get camera info!!\n");
		strcpy(info->path, path);
		info->resolution = 2; // 1280x720
		goto end;
	}else 
	{
		printf("error go get camer info jpg 1920x1080 and 1280x720!!\n");
		ret = -100;
		goto end;
	}


end:

	close(cam_fd);
	return ret;

}

int get_camera_info(char *path[], int path_num, t_camera_info *info)
{
	if(path==NULL || path_num <= 0 || info == NULL) 
	{
		printf("ERROR in path:%p, path_num = %d, info=%p\n",
				path, path_num, info);
		return -1;
	}

	int ret = 0;
	for(int i=0; i<path_num; i++)
	{
		ret = check_camera_path(path[i], info);
		if(ret == 0)
		{
			break;
		}
	}
	return ret;
}
	

