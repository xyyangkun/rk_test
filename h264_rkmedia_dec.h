/******************************************************************************
 *
 *       Filename:  h264_rkmedia_dec.h
 *
 *    Description: test rkmedia decode
 *
 *        Version:  1.0
 *        Created:  2022年11月09日 13时19分02秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef __H264_RKMEDIA_DEC_H__
#define __H264_RKMEDIA_DEC_H__

#ifdef __cplusplus
extern "C" {
#endif
// 使用rkmedia 解码h264 数据帧
void stop_test_h264_rkmedia_dec();
void start_test_h264_rkmedia_dec();

#ifdef __cplusplus
}
#endif

#endif//_H264_RKMEDIA_DEC_H_
