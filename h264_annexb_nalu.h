/******************************************************************************
 *
 *       Filename:  h264_annexb_nalu.h
 *
 *    Description:  解析h264
 *
 *        Version:  1.0
 *        Created:  2022年10月09日 12时29分43秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef _H264_ANNEXB_NALU_H_
#define _H264_ANNEXB_NALU_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>


static const uint8_t* h264_startcode(const uint8_t *data, size_t bytes)
{
	size_t i;
	for (i = 2; i + 1 < bytes; i++)
	{
		if (0x01 == data[i] && 0x00 == data[i - 1] && 0x00 == data[i - 2])
			return data + i + 1;
	}

	return NULL;
}

///@param[in] h264 H.264 byte stream format data(A set of NAL units)
static int mpeg4_h264_annexb_nalu(const void* h264, size_t bytes, void (*handler)(void* param, const uint8_t* nalu, size_t bytes), void* param)
{
	ptrdiff_t n;
	const uint8_t* p, *next, *end;

	end = (const uint8_t*)h264 + bytes;
	p = h264_startcode((const uint8_t*)h264, bytes);

	int hl;
	int _hl;
	
	_hl = p - (const uint8_t*)h264;
	while (p)
	{
		next = h264_startcode(p, (int)(end - p));
		if (next)
		{
			n = next - p - 3;
		}
		else
		{
			n = end - p;
		}
		hl = 3;
		
		while (n > 0 && 0 == p[n - 1]) // 是 00 00 00 01这种情况
		{
			n--; // filter tailing zero // 如果倒数第四位时0，长度要再减少1
			hl++; // 
		}
		assert(n > 0);
		if (n > 0)
		{
			//handler(param, p, (int)n);
			//handler(param, p-4, (int)(n+4)); // 可能存在 00 00 01这种情况
			handler(param, p-_hl, (int)(n+_hl));
		}
		_hl = hl;
		p = next;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif//_H264_ANNEXB_NALU_H_
