/******************************************************************************
 *
 *       Filename:  h264_annexb_file_nalu.h
 *
 *    Description:  从文件中读取 nalu 信息 
 *
 *        Version:  1.0
 *        Created:  2022年11月21日 16时19分10秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef _H264_ANNXB_FILE_NALU_H_
#define _H264_ANNXB_FILE_NALU_H_
// 创建线程读取h264文件，解析出帧数据，之后解码
int annxb_file_read(const char *filename, 
		void (*handler)(void *param, const uint8_t* nalu, size_t bytes),
		void *param);
#endif//_H264_ANNXB_FILE_NALU_H_
