/******************************************************************************
 *
 *       Filename:  librkmedia_vi_get_frame.h
 *
 *    Description:  get yuv for qt
 *
 *        Version:  1.0
 *        Created:  2022年11月10日 14时48分06秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef LIBRKMEDIA_VI_GET_FRAME_H_
#define LIBRKMEDIA_VI_GET_FRAME_H_

#ifdef __cplusplus
extern "C" {
#endif


// 获取yuv数据回调
typedef void (*OutYuvCb)(void *buf, unsigned int size);

// 初始化vi 
int init_vi(OutYuvCb cb);

// 释放vi, 停止回调
int deinit_vi();

#ifdef __cplusplus
}
#endif

#endif//LIBRKMEDIA_VI_GET_FRAME_H_
