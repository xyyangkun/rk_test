/******************************************************************************
 *
 *       Filename:  usb_camera_notify.h
 *
 *    Description:  usb camera 插入拔下检测，
 *    主要原理：检测/dev/ 下是否增加 或者 减少了 videox文件
 *
 *        Version:  1.0
 *        Created:  2021年12月20日 17时34分25秒
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
* \author yangkun
* \version 1.0
* \date 2021.12.20
*/
#ifndef _USB_CAMERA_NOTIFY_H_
#define _USB_CAMERA_NOTIFY_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef void* NHANDLE;

/**
 * @brief 通知usb 插入还是断开
 * @param[in] path usb路径
 * @param[in] cb_type  1 插入事件， 2 断开事件
 * @parma[in] fps  usb camera 获取帧率
 * @param[in] resolution  usb camera 支持的分辨率类型 1 1920x1080 2 1280x720
 * @param[in] handle 入参handle 
 */
typedef void (*usb_camera_notify_cb)(char *path, int cb_type, unsigned int resolution, unsigned int fps, long handle);


/**
 * @brief 初始化usb camera事件通知
 * @param[in] _cb usb camera插拔回调函数
 * @return 0 success, other failed
 */
int init_usb_camera_notify(NHANDLE *hd, usb_camera_notify_cb _cb);
/**
 * @brief 初始化usb camera事件通知
 * @param[in] _cb usb camera插拔回调函数
 * @param[in] handle 回调程序入参handle 
 * @param[in] path 如果摄像头已经打开，将打开的摄像头路径传给检测程序，检测程序在摄像头拔掉时会有通知
 * @return 0 success, other failed
 */
int init_usb_camera_notify_v1(NHANDLE *hd, usb_camera_notify_cb _cb, long handle, char *path);

/**
 * @brief 反初始化 usb camer事件通知
 * @return 0 success, other failed
 */
int deinit_usb_camera_notify(NHANDLE *hd);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif//_USB_CAMERA_NOTIFY_H_

