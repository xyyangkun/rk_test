/******************************************************************************
 *
 *       Filename:  usb_camera_get.h
 *
 *    Description:  usb camera info get
 *
 *        Version:  1.0
 *        Created:  2021年12月20日 16时05分32秒
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
* \author yangkun
* \version 1.0
* \date 2021.12.20
*/

#ifndef _USB_CAMERA_GET_H_
#define _USB_CAMERA_GET_H_


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct s_camera_info
{
	char path[100]; // 摄像头路径 如：/dev/video10
	int resolution; // 摄像头支持分辨率 1 1920x1080  2 1280x720  其他值不支持
	int frame_rate; // 帧率 默认30
}t_camera_info;


/**
 * @brief 通过传入camera 路径， 获取第一个满足要求的camera 路径，分辨率信息
 * @param[in] path  要获取的camera 路径数组
 * @param[in] path_num camera 路径数组的长度
 * @param[out] info 获取的第一个满足要求的camera
 * @return 0 succes, 获取满足要的camera, other 没有获取到满足要求的camera
 */
int check_camera_path(char *path, t_camera_info *info);

/**
 * @brief 通过传入camera 路径， 获取第一个满足要求的camera 路径，分辨率信息
 * @param[in] path  要获取的camera 路径数组
 * @param[in] path_num camera 路径数组的长度
 * @param[out] info 获取的第一个满足要求的camera
 * @return 0 succes, 获取满足要的camera, other 没有获取到满足要求的camera
 */
int get_camera_info(char *path[], int path_num, t_camera_info *info);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif//_USB_CAMERA_GET_H_
