/******************************************************************************
 *
 *       Filename:  uevent.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2022年10月12日 16时26分12秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef _UEVENT_H_
#define _UEVENT_H_
#ifdef __cplusplus
extern "C" {
#endif

struct uevent {
	const char *action;
	const char *path;
	const char *subsystem;
	const char *usb_state;
	const char *device_name;
};


// 启动sd卡检测线程
int start_uevent();
// 停止sd卡检测线程
int stop_uevent();

// 系统是否存在sd卡
extern bool g_have_sd;

#ifdef __cplusplus
}
#endif
#endif//_UEVENT_H_
