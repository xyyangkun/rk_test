/******************************************************************************
 *
 *       Filename:  test_rga.c
 *
 *    Description:  test rga
 *
 *        Version:  1.0
 *        Created:  2022年04月03日 22时52分25秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <rga/im2d.hpp>


#include <rga/RgaApi.h>
//#include "RgaApi.h"
#include "rkRgaApi.h"
#include "easymedia/rkmedia_api.h"

#define ALIGN16(value) ((value + 15) & (~15))


static void blend_buffer(void *src_p, int src_fd, int src_w, int src_h, int src_fmt,
                         void *dst_p, int dst_fd, int dst_w, int dst_h, int dst_fmt,
						 void *src1_p, int src1_fd, int src1_w, int src1_h, int src1_fmt)
{
	int ret;
	rga_buffer_t    src;
	rga_buffer_t    src1;
	rga_buffer_t    dst;

	im_rect         src_rect;
	im_rect         src1_rect;
	im_rect         dst_rect;

	memset(&src_rect, 0, sizeof(src_rect));
	memset(&src1_rect, 0, sizeof(src_rect));
	memset(&dst_rect, 0, sizeof(dst_rect));

	memset(&src, 0, sizeof(src));
	memset(&src1, 0, sizeof(src));
	memset(&dst, 0, sizeof(dst));

#if 0
	src = wrapbuffer_physicaladdr(src_p, src_w, src_h, src_fmt);
	src1 = wrapbuffer_physicaladdr(src1_p, src1_w, src1_h, src1_fmt);
	dst = wrapbuffer_physicaladdr(dst_p, dst_w, dst_h, dst_fmt);
#else
	src =  wrapbuffer_fd(src_fd, src_w, src_h, src_fmt);
	src1 = wrapbuffer_fd(src1_fd, src1_w, src1_h, src1_fmt);
	dst =  wrapbuffer_fd(dst_fd, dst_w, dst_h, dst_fmt);
#endif

#if 0
	ret = imcheck_composite(src, dst, src1, src_rect, dst_rect, src1_rect, IM_ALPHA_BLEND_DST_OVER);
	if (IM_STATUS_NOERROR != ret) {
		printf("error in check\n");
		exit(1);
	}
#endif
	ret = imcomposite(src, src1, dst, IM_ALPHA_BLEND_DST_OVER);
	//ret = imcomposite(src, src1, src, IM_ALPHA_BLEND_DST_OVER);
	if (IM_STATUS_SUCCESS != ret) {
		printf("error in check:%d\n", ret);
		//exit(1);
	}
}



int main()
{
	unsigned int src1_w, src1_h;
	unsigned int src2_w, src2_h;
	unsigned int dst_w,  dst_h ;
	IMAGE_TYPE_E src1_type, src2_type, dst_type;
	MEDIA_BUFFER mb_src1, mb_src2, mb_dst;

	src1_w = 1920;
	src1_h = 1080;
	src1_type = IMAGE_TYPE_NV12;

	src2_w = 1280;
	src2_h = 720;
	src2_type = IMAGE_TYPE_ARGB8888;//RK_FORMAT_RGBA_8888;

	dst_w = 1280;
	dst_h = 720;
	dst_type = IMAGE_TYPE_NV12;
	//dst_type = IMAGE_TYPE_ARGB8888;
	// 分配内存
	{
		MB_IMAGE_INFO_S disp_info= {src1_w, src1_h,
			src1_w, src1_h,
			src1_type};

		mb_src1 = RK_MPI_MB_CreateImageBuffer(&disp_info, RK_TRUE, 0);
		if (!mb_src1) {
			printf("ERROR: no space left!, src1_mb\n");
			exit(1);
		}
	}
	{
		MB_IMAGE_INFO_S disp_info= {src2_w, src2_h,
			src2_w, src2_h,
				src2_type};

		mb_src2 = RK_MPI_MB_CreateImageBuffer(&disp_info, RK_TRUE, 0);
		if (!mb_src2) {
			printf("ERROR: no space left!, src2_mb\n");
			exit(1);
		}
	}


	{
		MB_IMAGE_INFO_S disp_info= {dst_w, dst_h,
			dst_w, dst_h,
			dst_type};

		mb_dst = RK_MPI_MB_CreateImageBuffer(&disp_info, RK_TRUE, 0);
		if (!mb_dst) {
			printf("ERROR: no space left!, dst_mb\n");
			exit(1);
		}
	}

	// 读取数据
	{
		// src1
		const char *src1 = "src1.yuv";
		FILE *fp = fopen(src1, "rb");
		assert(fp);
		fseek(fp, 0, SEEK_END);
		int size = ftell(fp);
		printf("src1 file size:%d\n", size);
		fseek(fp, 0, SEEK_SET);
		assert(size == src1_w*src1_h*3/2);
		int ret = fread(RK_MPI_MB_GetPtr(mb_src1), 1, size, fp);
		assert(ret == size);
		fclose(fp);
	}

	{
		// src2
		const char *src2 = "src2.rgb";
		FILE *fp = fopen(src2, "rb");
		assert(fp);
		fseek(fp, 0, SEEK_END);
		int size = ftell(fp);
		printf("src2 file size:%d\n", size);
		fseek(fp, 0, SEEK_SET);
		assert(size == src2_w*src2_h*4);
		int ret = fread(RK_MPI_MB_GetPtr(mb_src2), 1, size, fp);
		assert(ret == size);
		fclose(fp);
	}


	// 调用
	blend_buffer(
			RK_MPI_MB_GetPtr(mb_src1),RK_MPI_MB_GetFD(mb_src1), src1_w, src1_h, RK_FORMAT_YCrCb_420_SP,
			RK_MPI_MB_GetPtr(mb_dst), RK_MPI_MB_GetFD(mb_dst),  dst_w, dst_h, RK_FORMAT_YCrCb_420_SP/*RK_FORMAT_BGRA_8888*/,
			RK_MPI_MB_GetPtr(mb_src2),RK_MPI_MB_GetFD(mb_src2), src2_w, src2_h, RK_FORMAT_BGRA_8888);


	// 将结果写入内存
	{
		// dst
		const char *dst = "dst.yuv";
		FILE *fp = fopen(dst, "wb");
		assert(fp);
		int size = src2_w*src2_h*3/2;
		int ret = fwrite(RK_MPI_MB_GetPtr(mb_dst), 1, size, fp);
		assert(ret == size);
		fclose(fp);
	}
	{
		// src1
		const char *dst = "dst1.yuv";
		FILE *fp = fopen(dst, "wb");
		assert(fp);
		int size = src2_w*src2_h*4;
		int ret = fwrite(RK_MPI_MB_GetPtr(mb_src2), 1, size, fp);
		assert(ret == size);
		fclose(fp);
	}


	return 0;
}
