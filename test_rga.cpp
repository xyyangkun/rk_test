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


#include <rga/RgaApi.h>
//#include "RgaApi.h"
#include "rkRgaApi.h"
#include "easymedia/rkmedia_api.h"

#define ALIGN16(value) ((value + 15) & (~15))



static void blend_buffer(void *src_p, int src_w, int src_h, int src_fmt,
                         void *dst_p, int dst_w, int dst_h, int dst_fmt,
						 void *src1_p, int src1_w, int src1_h, int src1_fmt,
                         int rotation) {
  struct rkRgaCfg src_cfg, src1_cfg, dst_cfg;

  src_cfg.addr = src_p;
  src_cfg.fmt = src_fmt;//RK_FORMAT_YCrCb_420_SP;
  src_cfg.width = src_w;
  src_cfg.height = src_h;

  src1_cfg.addr = src1_p;
  src1_cfg.fmt = src1_fmt;//RK_FORMAT_RGBA_8888 RK_FORMAT_BGRA_8888
  src1_cfg.width = src1_w;
  src1_cfg.height = src1_h;

  dst_cfg.addr = dst_p;
  dst_cfg.fmt = dst_fmt;//RK_FORMAT_YCrCb_420_SP;
  dst_cfg.width = dst_w;
  dst_cfg.height = dst_h;

  if (0 != rkRgaBlit1(&src_cfg, &dst_cfg, &src1_cfg))
    printf("%s: rga fail\n", __func__);
}


static void _rga_copy(void *src, void *dst, int w, int h) {
  struct rkRgaCfg src_cfg, dst_cfg;

  src_cfg.addr = src;
  src_cfg.fmt = RK_FORMAT_YCrCb_420_SP;
  src_cfg.width = w;
  src_cfg.height = h;

  dst_cfg.addr = dst;
  dst_cfg.fmt = RK_FORMAT_YCrCb_420_SP;
  dst_cfg.width = w;
  dst_cfg.height = h;
  rkRgaBlit(&src_cfg, &dst_cfg);
}


static void scale_buffer(void *src_p, int src_w, int src_h, int src_fmt,
                         void *dst_p, int dst_w, int dst_h, int dst_fmt,
                         int rotation) {
  rga_info_t src, dst;
  memset(&src, 0, sizeof(rga_info_t));
  src.fd = -1;
  src.virAddr = src_p;
  src.mmuFlag = 1;
  src.rotation = rotation;
  rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, src_fmt);
  memset(&dst, 0, sizeof(rga_info_t));
  dst.fd = -1;
  dst.virAddr = dst_p;
  dst.mmuFlag = 1;
  rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, ALIGN16(dst_w), ALIGN16(dst_h), dst_fmt);
  if (c_RkRgaBlit(&src, &dst, NULL))
    printf("%s: rga fail\n", __func__);
}

static void scale_buffer1(void *src_p, int src_w, int src_h, int src_fmt,
                         void *dst_p, int dst_w, int dst_h, int dst_fmt,
                         int rotation) {
  rga_info_t src, dst;
  memset(&src, 0, sizeof(rga_info_t));
  src.fd = -1;
  src.virAddr = src_p;
  src.mmuFlag = 1;
  src.rotation = rotation;
  rga_set_rect(&src.rect, 0, 0, src_w, src_h, ALIGN16(src_w), ALIGN16(src_h), src_fmt);
  memset(&dst, 0, sizeof(rga_info_t));
  dst.fd = -1;
  dst.virAddr = dst_p;
  dst.mmuFlag = 1;
  rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, ALIGN16(dst_w), ALIGN16(dst_h), dst_fmt);
  if (c_RkRgaBlit(&src, &dst, NULL))
    printf("%s: rga fail\n", __func__);
}



static void scale_buffer_copy(void *src_p, int src_w, int src_h, int src_fmt,
                         void *dst_p, int dst_w, int dst_h, int dst_fmt,
                         int rotation, void *src1_p) {
  rga_info_t src, src1, dst;
  memset(&src, 0, sizeof(rga_info_t));
  src.fd = -1;
  src.virAddr = src_p;
  src.mmuFlag = 1;
  //src.rotation = rotation;
  rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, src_fmt);

  memset(&dst, 0, sizeof(rga_info_t));
  dst.fd = -1;
  dst.virAddr = dst_p;
  dst.mmuFlag = 1;
  rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dst_fmt);

  memset(&src1, 0, sizeof(rga_info_t));
  src1.fd = -1;
  src1.virAddr = src1_p;
  src1.mmuFlag = 1;
  //src.rotation = rotation;
  rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dst_fmt);


  if (c_RkRgaBlit(&src, &dst, &src1))
    printf("%s: rga fail\n", __func__);
}

static void copy_buffer(void *src_p, int src_w, int src_h, int src_fmt,
                         void *dst_p, int dst_w, int dst_h, int dst_fmt,
                         int rotation) {
  rga_info_t src, dst;
  memset(&src, 0, sizeof(rga_info_t));
  src.fd = -1;
  src.virAddr = src_p;
  src.mmuFlag = 1;
  src.rotation = rotation;
  rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, src_fmt);
  memset(&dst, 0, sizeof(rga_info_t));
  dst.fd = -1;
  dst.virAddr = dst_p;
  dst.mmuFlag = 1;
  rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dst_fmt);
  if (c_RkRgaBlit(&src, &dst, NULL))
    printf("%s: rga fail\n", __func__);
}

static void compose_buffer(void *src_p, int src_w, int src_h, int src_fmt,
                           void *dst_p, int dst_w, int dst_h, int dst_fmt,
                           int x, int y, int w, int h) {
  rga_info_t src, dst;
  memset(&src, 0, sizeof(rga_info_t));
  src.fd = -1;
  src.virAddr = src_p;
  src.mmuFlag = 1;
  rga_set_rect(&src.rect, 0, 0, src_w, src_h, ALIGN16(src_w), ALIGN16(src_h), src_fmt);
  memset(&dst, 0, sizeof(rga_info_t));
  dst.fd = -1;
  dst.virAddr = dst_p;
  dst.mmuFlag = 1;
  rga_set_rect(&dst.rect, x, y, w, h, ALIGN16(dst_w), ALIGN16(dst_h), dst_fmt);
  if (c_RkRgaBlit(&src, &dst, NULL))
    printf("%s: rga fail\n", __func__);
}

static void compose_buffer_blend(void *src_p, int src_w, int src_h, int src_fmt,
                           void *dst_p, int dst_w, int dst_h, int dst_fmt,
                           int x, int y, int w, int h, unsigned int blend) {
  rga_info_t src, dst;
  memset(&src, 0, sizeof(rga_info_t));
  src.fd = -1;
  src.virAddr = src_p;
  src.mmuFlag = 1;
  src.blend = blend;
  rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, src_fmt);
  memset(&dst, 0, sizeof(rga_info_t));
  dst.fd = -1;
  dst.virAddr = dst_p;
  dst.mmuFlag = 1;
  rga_set_rect(&dst.rect, x, y, w, h, dst_w, dst_h, dst_fmt);
  if (c_RkRgaBlit(&src, &dst, NULL))
    printf("%s: rga fail\n", __func__);
}

static void compose_buffer_copy(void *src_p, int src_w, int src_h, int src_fmt,
                           void *dst_p, int dst_w, int dst_h, int dst_fmt,
                           int x, int y, int w, int h, void *src1_p) {
  rga_info_t src, src1, dst;
  memset(&src, 0, sizeof(rga_info_t));
  src.fd = -1;
  src.virAddr = src_p;
  src.mmuFlag = 1;
  rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, src_fmt);

  memset(&dst, 0, sizeof(rga_info_t));
  dst.fd = -1;
  dst.virAddr = dst_p;
  dst.mmuFlag = 1;
  rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dst_fmt);

  memset(&src1, 0, sizeof(rga_info_t));
  src1.fd = -1;
  src1.virAddr = src1_p;
  src1.mmuFlag = 1;
  rga_set_rect(&src1.rect, 200, 200, src_w, src_h, dst_w, dst_h, dst_fmt);



  if (c_RkRgaBlit(&src, &dst, &src1))
    printf("%s: rga fail\n", __func__);
}



int main()
{
	unsigned int src1_w, src1_h;
	unsigned int src2_w, src2_h;
	unsigned int dst_w,  dst_h ;
	IMAGE_TYPE_E src1_type, src2_type, dst_type;
	MEDIA_BUFFER mb_src1, mb_src2, mb_dst;

	src1_w = 1280;
	src1_h = 720;
	src1_type = IMAGE_TYPE_NV12;

	src2_w = 1280;
	src2_h = 720;
	src2_type = IMAGE_TYPE_ARGB8888;//RK_FORMAT_RGBA_8888;

	dst_w = 1280;
	dst_h = 720;
	//dst_type = IMAGE_TYPE_NV12;
	dst_type = IMAGE_TYPE_ARGB8888;
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



	// 对数据进行操作
	blend_buffer(
			RK_MPI_MB_GetPtr(mb_src1), src1_w, src1_h, RK_FORMAT_YCrCb_420_SP,
			RK_MPI_MB_GetPtr(mb_dst), dst_w, dst_h, RK_FORMAT_RGBA_8888 /*RK_FORMAT_BGRA_8888*/,
			RK_MPI_MB_GetPtr(mb_src2), src2_w, src2_h, RK_FORMAT_YCrCb_420_SP,
			0);

	// 将结果写入内存
	{
		// src1
		const char *dst = "dst.yuv";
		FILE *fp = fopen(dst, "wb");
		assert(fp);
		int size = src2_w*src2_h*4;
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

	// 释放内存
	RK_MPI_MB_ReleaseBuffer(mb_src1);
	RK_MPI_MB_ReleaseBuffer(mb_src2);
	RK_MPI_MB_ReleaseBuffer(mb_dst);

}
