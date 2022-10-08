/******************************************************************************
 *
 *       Filename:  h264_dec.c
 *
 *    Description:  test h264 dec
 *
 *        Version:  1.0
 *        Created:  2022年10月08日 17时55分51秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stddef.h>
#include <assert.h>
#include "h264_dec.h"

static uint8_t* file_read(const char* file, long* size)
{
	FILE* fp = fopen(file, "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		*size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		uint8_t* ptr = (uint8_t*)malloc(*size);
		fread(ptr, 1, *size, fp);
		fclose(fp);

		return ptr;
	}

	return NULL;
}


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
int mpeg4_h264_annexb_nalu(const void* h264, size_t bytes, void (*handler)(void* param, const uint8_t* nalu, size_t bytes), void* param)
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

// 获得h264 帧，可以进行解码
static void h264_handler(void* param, const uint8_t* nalu, size_t bytes)
{
	//printf("===============>yk debug!, param=%p==>nalu %d== %02x %02x %02x %02x\n", param, bytes, nalu[0], nalu[1], nalu[2], nalu[3]);
	FILE *fp=(FILE*)param;
	if(fp)fwrite(nalu, bytes, 1, fp);

}

static int is_start = 1;
static pthread_t thread_dec;
static void *test_h264_dec_proc(void *param)
{
	char *h264 = "./tennis200.h264";
	
	FILE* fp = fopen("out.h264", "wb+");
	int ret;

	// 读取h264文件
	long bytes = 0;
	uint8_t* ptr = file_read(h264, &bytes);
	while(is_start == 1) {
		uint8_t *p = ptr;
		// 解析h264文件
		ret = mpeg4_h264_annexb_nalu(p, bytes, h264_handler, (void*)fp);
		if(ret == 0)break;

	}

	if(fp){
		fclose(fp);
	}

	free(ptr);
}

void stop_test_h264_dec()
{
	is_start = 0;
	pthread_join(thread_dec, NULL);

}

void start_test_h264_dec()
{
	is_start = 1;
	int ret = pthread_create(&thread_dec, NULL, test_h264_dec_proc, NULL);	
	if(ret!=0 )
	{
		printf("error to create thread!\n");
		exit(0);
	}
}
