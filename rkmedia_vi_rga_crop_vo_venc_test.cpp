// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// 1080p /dev/video5 输出， 随机裁切1080 720，同时编码，输出在1080 720中间切换

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <rga/im2d.h>
#include <rga/rga.h>
#include "rkmedia_api.h"
#include "rkmedia_venc.h"

#define SCREEN_WIDTH 1920 
#define SCREEN_HEIGHT 1080
int init_vo(int w, int h, const char *u32PlaneType);
int deinit_vo();
int init_venc();
int deinit_venc();

MEDIA_BUFFER_POOL mbp;

typedef struct rga_demo_arg_s {
  int target_x;
  int target_y;
  int target_width;
  int target_height;
  int vi_width;
  int vi_height;
} rga_demo_arg_t;

static bool quit = false;
IMAGE_TYPE_E g_enPixFmt = IMAGE_TYPE_NV12;
MPP_CHN_S g_stViChn;
MPP_CHN_S g_stVencChn;

static void sigterm_handler(int sig) {
	fprintf(stderr, "signal %d\n", sig);
	quit = true;
}

void change_resolution(MB_IMAGE_INFO_S &stImageInfo, int &w, int &h)
{
	static int count =0;
	static int count1 =0;

	struct timespec startT, endT;
	uint64_t delta_us;
	uint32_t delta_ms;

	const char *u32PlaneType = "Overlay";
	
	int ret;

	count++;
	if(count%240 == 0)
	{
		printf("count=%d\n", count);
		count1++;
		if(count1%2==1)
		{
			clock_gettime(CLOCK_MONOTONIC, &startT);
			VENC_RESOLUTION_PARAM_S stResolutionParam = {1280, 720, 1280, 720};
			ret =  RK_MPI_VENC_SetResolution(0, stResolutionParam);
			if(ret != 0)
			{
				printf("xxxxxxxxxxxxxxx => 1error in set resolution!!!!\n");
				exit(0);
			}
			ret =  RK_MPI_VENC_SetResolution(1, stResolutionParam);
			if(ret != 0)
			{
				printf("xxxxxxxxxxxxxxx => 2error in set resolution!!!!\n");
				exit(0);
			}

			clock_gettime(CLOCK_MONOTONIC, &endT);
			delta_us = (endT.tv_sec - startT.tv_sec) * 1000000 + (endT.tv_nsec - startT.tv_nsec) / 1000;
			delta_ms = (endT.tv_sec - startT.tv_sec) * 1000 + (endT.tv_nsec - startT.tv_nsec) / 1000000;
			printf("720p chang resolution ==> %lu us, %d ms\n", delta_us, delta_ms);

			// 设置720p50
			clock_gettime(CLOCK_MONOTONIC, &startT);
			deinit_vo();
			clock_gettime(CLOCK_MONOTONIC, &endT);

			delta_us = (endT.tv_sec - startT.tv_sec) * 1000000 + (endT.tv_nsec - startT.tv_nsec) / 1000;
			delta_ms = (endT.tv_sec - startT.tv_sec) * 1000 + (endT.tv_nsec - startT.tv_nsec) / 1000000;
			printf("720p deinit vo ==> %lu us, %d ms\n", delta_us, delta_ms);

			w=1280;
			h = 720;
			clock_gettime(CLOCK_MONOTONIC, &startT);
			init_vo(w, h, u32PlaneType);

			clock_gettime(CLOCK_MONOTONIC, &endT);
			delta_us = (endT.tv_sec - startT.tv_sec) * 1000000 + (endT.tv_nsec - startT.tv_nsec) / 1000;
			delta_ms = (endT.tv_sec - startT.tv_sec) * 1000 + (endT.tv_nsec - startT.tv_nsec) / 1000000;
			printf("720p init vo ==> %lu us, %d ms\n", delta_us, delta_ms);

			// resize image wxh.
			stImageInfo.u32Width = w;
			stImageInfo.u32Height = h;
			stImageInfo.u32HorStride = w;
			stImageInfo.u32VerStride = h;
			stImageInfo.enImgType = IMAGE_TYPE_NV12;

			VENC_CHN_STATUS_S Status = {0};
			ret = RK_MPI_VENC_QueryStatus(0, &Status);
			if(ret != 0)
			{
				printf("error in query status!\n");
				exit(0);
			}
			printf("change 720p statue, leftframe=%d, totalframes:%d leftpackets:%d, totalpackets:%d\n",
					Status.u32LeftFrames, Status.u32TotalFrames, Status.u32LeftPackets, Status.u32TotalPackets);
		}
		if(count1%2==0)
		{
			clock_gettime(CLOCK_MONOTONIC, &startT);
			VENC_RESOLUTION_PARAM_S stResolutionParam = {1920, 1080, 1920, 1080};
			ret =  RK_MPI_VENC_SetResolution(0, stResolutionParam);
			if(ret != 0)
			{
				printf("xxxxxxxxxxxxxxx => 3error in set resolution!!!!\n");
				exit(0);
			}
			ret =  RK_MPI_VENC_SetResolution(1, stResolutionParam);
			if(ret != 0)
			{
				printf("xxxxxxxxxxxxxxx => 4error in set resolution!!!!\n");
				exit(0);
			}
			clock_gettime(CLOCK_MONOTONIC, &endT);
			delta_us = (endT.tv_sec - startT.tv_sec) * 1000000 + (endT.tv_nsec - startT.tv_nsec) / 1000;
			delta_ms = (endT.tv_sec - startT.tv_sec) * 1000 + (endT.tv_nsec - startT.tv_nsec) / 1000000;
			printf("720p chang resolution ==> %lu us, %d ms\n", delta_us, delta_ms);


			// 设置1080p60
			clock_gettime(CLOCK_MONOTONIC, &startT);
			deinit_vo();
			clock_gettime(CLOCK_MONOTONIC, &endT);

			delta_us = (endT.tv_sec - startT.tv_sec) * 1000000 + (endT.tv_nsec - startT.tv_nsec) / 1000;
			delta_ms = (endT.tv_sec - startT.tv_sec) * 1000 + (endT.tv_nsec - startT.tv_nsec) / 1000000;
			printf("1080p deinit vo ==> %lu us, %d ms\n", delta_us, delta_ms);

			w=1920;
			h = 1080;
			clock_gettime(CLOCK_MONOTONIC, &startT);
			init_vo(w, h, u32PlaneType);
			clock_gettime(CLOCK_MONOTONIC, &endT);

			delta_us = (endT.tv_sec - startT.tv_sec) * 1000000 + (endT.tv_nsec - startT.tv_nsec) / 1000;
			delta_ms = (endT.tv_sec - startT.tv_sec) * 1000 + (endT.tv_nsec - startT.tv_nsec) / 1000000;
			printf("1080p deinit vo ==> %lu us, %d ms\n", delta_us, delta_ms);

			// resize image wxh.
			stImageInfo.u32Width = w;
			stImageInfo.u32Height = h;
			stImageInfo.u32HorStride = w;
			stImageInfo.u32VerStride = h;
			stImageInfo.enImgType = IMAGE_TYPE_NV12; 

			VENC_CHN_STATUS_S Status = {0};
			ret = RK_MPI_VENC_QueryStatus(0, &Status);
			if(ret != 0)
			{
				printf("error in query status!\n");
				exit(0);
			}
			printf("change 1080p statue, leftframe=%d, totalframes:%d leftpackets:%d, totalpackets:%d\n",
					Status.u32LeftFrames, Status.u32TotalFrames, Status.u32LeftPackets, Status.u32TotalPackets);

		}
		clock_gettime(CLOCK_MONOTONIC, &endT);

		delta_us = (endT.tv_sec - startT.tv_sec) * 1000000 + (endT.tv_nsec - startT.tv_nsec) / 1000;
		delta_ms = (endT.tv_sec - startT.tv_sec) * 1000 + (endT.tv_nsec - startT.tv_nsec) / 1000000;
		printf("==> %lu us, %d ms\n", delta_us, delta_ms);
	}
}

static void *GetVencBuffer(void *arg) {
	printf("#Start %s thread, arg:%p\n", __func__, arg);

	char *ot_path = (char *)arg;
	printf("#Start %s thread, arg:%p, out path: %s\n", __func__, arg, ot_path);
	FILE *save_file = fopen(ot_path, "w");
	if (!save_file)
		printf("ERROR: Open %s failed!\n", ot_path);



	MEDIA_BUFFER mb = NULL;
	while (!quit) {
		mb = RK_MPI_SYS_GetMediaBuffer(g_stVencChn.enModId, g_stVencChn.s32ChnId,
				-1);
		if (mb) {
			printf(
					"-Get Video Encoded packet():ptr:%p, fd:%d, size:%zu, mode:%d, time "
					"= %llu.\n",
					RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb), RK_MPI_MB_GetSize(mb),
					RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetTimestamp(mb));

			if (save_file)
				fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), save_file);
			RK_MPI_MB_ReleaseBuffer(mb);
		}
	}

	if (save_file)
		fclose(save_file);

	return NULL;
}

static void *GetJpgMediaBuffer(void *arg) {
	char *ot_path = (char *)arg;
	FILE *save_file = NULL;
	if(arg) {
		printf("#Start %s thread, arg:%p, out path: %s\n", __func__, arg, ot_path);
		save_file = fopen(ot_path, "wb");
		if (!save_file)
		{
			printf("ERROR: Open %s failed!\n", ot_path);
			exit(1);
		}
	}

	MEDIA_BUFFER mb = NULL;
	while (!quit) {
		mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_VENC, 1, 500);
		if (!mb) {
			printf("in %s get null buffer!or timeout 500ms, quit:%d\n", __FUNCTION__, quit);
			usleep(1000);
			if(quit) {
				break;
			}
			continue;
		}
		/*
		   printf("Get packet:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
		   "timestamp:%lld\n",
		   RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb), RK_MPI_MB_GetSize(mb),
		   RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetChannelID(mb),
		   RK_MPI_MB_GetTimestamp(mb));
		   */
#if 0
		static int i=0;
		if(100==i++)
		{
			FILE *_save_file = fopen("/tmp/out1000.jpg", "wb");
			if(_save_file)
			{
				fwrite(RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb), 1, _save_file);
				fclose(_save_file);
			}
			else
			{
				printf("error !!! size:%d\n", RK_MPI_MB_GetSize(mb));
				exit(1);
			}
		}
		printf("i====================================>%d\n", i);
#endif

		if (save_file)
			fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), save_file);
		RK_MPI_MB_ReleaseBuffer(mb);
	}
	exit(1);

	if (save_file)
		fclose(save_file);

	printf("%s exit %d\n", __func__, __LINE__);
	return NULL;
}

static void *GetMediaBuffer(void *arg) {
  printf("#Start %s thread, arg:%p\n", __func__, arg);
  rga_demo_arg_t *crop_arg = (rga_demo_arg_t *)arg;
  int ret;
  rga_buffer_t src;
  rga_buffer_t dst;
  MEDIA_BUFFER src_mb = NULL;
  MEDIA_BUFFER dst_mb = NULL;
  printf("test, %d, %d, %d, %d\n", crop_arg->target_height,
         crop_arg->target_width, crop_arg->vi_height, crop_arg->vi_width);

  MB_IMAGE_INFO_S stImageInfo = {1920, 1080, 1920, 1080, IMAGE_TYPE_NV12};
  int w=1920;
  int h = 1080;

  while (!quit) {
	  static int count = 0;;
	  count++;
	  printf("=====================================>%d\n", count);
    src_mb =
        RK_MPI_SYS_GetMediaBuffer(g_stViChn.enModId, g_stViChn.s32ChnId, -1);
    if (!src_mb) {
      printf("============> ERROR: RK_MPI_SYS_GetMediaBuffer get null buffer!\n");
      break;
    }

	change_resolution(stImageInfo, w, h);

	crop_arg->target_width = w;
	crop_arg->target_height = h;
	crop_arg->target_width = w;
	crop_arg->target_height = h;

	/*
    MB_IMAGE_INFO_S stImageInfo = {
        crop_arg->target_width, crop_arg->target_height, crop_arg->target_width,
        crop_arg->target_height, IMAGE_TYPE_NV12};
		*/
    //dst_mb = RK_MPI_MB_CreateImageBuffer(&stImageInfo, RK_TRUE, 0);
	if (!mbp) {
		printf("===========> error 1Create buffer pool for vo failed!\n");
		exit(1);
	}
    dst_mb = RK_MPI_MB_POOL_GetBuffer(mbp, RK_TRUE);
    if (!dst_mb) {
      printf("==============> ERROR: BufferPool get null buffer...\n");
      //usleep(10000);
	  exit(1);
    }



	printf("%d %d %d %d %d\n", stImageInfo.u32Width, stImageInfo.u32Height, stImageInfo.u32HorStride, stImageInfo.u32VerStride, stImageInfo.enImgType);
	// 转换为对应分辨率的mb 
	dst_mb = RK_MPI_MB_ConvertToImgBuffer(dst_mb, &stImageInfo);
	if (!dst_mb) {
		printf("===========> ERROR: convert to img buffer failed!\n");
		exit(1);
	}
	int u32FrameSize = (RK_U32)(w* h* 3/2);
    RK_MPI_MB_SetSize(dst_mb, u32FrameSize);


    RK_MPI_MB_SetTimestamp(dst_mb, RK_MPI_MB_GetTimestamp(src_mb));
    if (!dst_mb) {
      printf("ERROR: RK_MPI_MB_CreateImageBuffer get null buffer!\n");
      break;
    }

    src = wrapbuffer_fd(RK_MPI_MB_GetFD(src_mb), crop_arg->vi_width,
                        crop_arg->vi_height, RK_FORMAT_YCbCr_420_SP);
    dst = wrapbuffer_fd(RK_MPI_MB_GetFD(dst_mb), crop_arg->target_width,
                        crop_arg->target_height, RK_FORMAT_YCbCr_420_SP);
    im_rect src_rect = {crop_arg->target_x, crop_arg->target_y,
                        crop_arg->target_width, crop_arg->target_height};
    im_rect dst_rect = {0};
    ret = imcheck(src, dst, src_rect, dst_rect, IM_CROP);
    if (IM_STATUS_NOERROR != ret) {
      printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
      break;
    }
    IM_STATUS STATUS = imcrop(src, dst, src_rect);
    if (STATUS != IM_STATUS_SUCCESS)
      printf("imcrop failed: %s\n", imStrError(STATUS));

    VENC_RESOLUTION_PARAM_S stResolution;
    stResolution.u32Width = crop_arg->target_width;
    stResolution.u32Height = crop_arg->target_height;
    stResolution.u32VirWidth = crop_arg->target_width;
    stResolution.u32VirHeight = crop_arg->target_height;

    RK_MPI_VENC_SetResolution(g_stVencChn.s32ChnId, stResolution);
    ret = RK_MPI_SYS_SendMediaBuffer(g_stVencChn.enModId, g_stVencChn.s32ChnId,
                               dst_mb);
	if (ret) {
		printf("ERROR: RK_MPI_SYS_SendMediaBuffer to VENC[0] failed! ret=%d\n",
				ret);
		RK_MPI_MB_ReleaseBuffer(dst_mb);
		// break;
		exit(1);
	}
#if 1
	if(count%2==0)
	{
		ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_VENC, 1, dst_mb);
		if (ret) {
			printf("ERROR: RK_MPI_SYS_SendMediaBuffer to VENC[1] failed! ret=%d\n",
					ret);
			RK_MPI_MB_ReleaseBuffer(dst_mb);
			// break;
			exit(1);
		}
	}
#endif

#if 1
	ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_VO, 0, dst_mb);
	if (ret) {
		printf("ERROR: RK_MPI_SYS_SendMediaBuffer to VO[0] failed! ret=%d\n",
				ret);
		RK_MPI_MB_ReleaseBuffer(dst_mb);
		// break;
		exit(1);
	}
#endif
    RK_MPI_MB_ReleaseBuffer(src_mb);
    RK_MPI_MB_ReleaseBuffer(dst_mb);
    src_mb = NULL;
    dst_mb = NULL;
  }

  if (src_mb)
    RK_MPI_MB_ReleaseBuffer(src_mb);
  if (dst_mb)
    RK_MPI_MB_ReleaseBuffer(dst_mb);

  return NULL;
}

int deinit_venc()
{
	RK_MPI_VENC_DestroyChn(0);
	RK_MPI_VENC_DestroyChn(1);
	return 0;
}

int init_venc()
{
	RK_U32 u32Width = 1920;
	RK_U32 u32Height = 1080;
	int ret;

	RK_U32 u32Fps = 30;
	VENC_RC_MODE_E enEncoderMode = VENC_RC_MODE_H264CBR;
	VENC_CHN_ATTR_S venc_chn_attr;
	memset(&venc_chn_attr, 0, sizeof(VENC_CHN_ATTR_S));
	venc_chn_attr.stVencAttr.u32PicWidth = u32Width;
	venc_chn_attr.stVencAttr.u32PicHeight = u32Height;
	venc_chn_attr.stVencAttr.u32VirWidth = u32Width;
	venc_chn_attr.stVencAttr.u32VirHeight = u32Height;
	venc_chn_attr.stVencAttr.imageType = IMAGE_TYPE_NV12;
	if (enEncoderMode == VENC_RC_MODE_H264CBR) {
		venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H264;
		venc_chn_attr.stVencAttr.u32Profile = 77;
		venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
		venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 2 * u32Fps;
		venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = u32Width * u32Height;
		venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 1;
		venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = u32Fps;
		venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 1;
		venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = u32Fps;
	} else if (enEncoderMode == VENC_RC_MODE_H265CBR) {
		venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H265;
		venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
		venc_chn_attr.stRcAttr.stH265Cbr.u32Gop = 2 * u32Fps;
		venc_chn_attr.stRcAttr.stH265Cbr.u32BitRate = u32Width * u32Height;
		venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateDen = 1;
		venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateNum = u32Fps;
		venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateDen = 1;
		venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateNum = u32Fps;
	} else {
		venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_MJPEG;
		venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGCBR;
		venc_chn_attr.stRcAttr.stMjpegCbr.fr32DstFrameRateDen = 1;
		venc_chn_attr.stRcAttr.stMjpegCbr.fr32DstFrameRateNum = u32Fps;
		venc_chn_attr.stRcAttr.stMjpegCbr.u32SrcFrameRateDen = 1;
		venc_chn_attr.stRcAttr.stMjpegCbr.u32SrcFrameRateNum = u32Fps;
		venc_chn_attr.stRcAttr.stMjpegCbr.u32BitRate = u32Width * u32Height * 8;
	}
	ret = RK_MPI_VENC_CreateChn(0, &venc_chn_attr);
	if (ret) {
		printf("ERROR: Create venc failed!\n");
		exit(0);
	}

	if(1)
	{
		int chn = 1;
		int u32Width = 1920;
		int u32Height = 1080;

		VENC_CHN_ATTR_S venc_chn_attr;
		memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
		venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_MJPEG;
		venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_MJPEGCBR;
		venc_chn_attr.stRcAttr.stMjpegCbr.fr32DstFrameRateDen = 1;
		// venc_chn_attr.stRcAttr.stMjpegCbr.fr32DstFrameRateNum = 60;
		venc_chn_attr.stRcAttr.stMjpegCbr.fr32DstFrameRateNum = 30;
		venc_chn_attr.stRcAttr.stMjpegCbr.u32SrcFrameRateDen = 1;
		// venc_chn_attr.stRcAttr.stMjpegCbr.u32SrcFrameRateNum = 60;
		venc_chn_attr.stRcAttr.stMjpegCbr.u32SrcFrameRateNum = 30;
		//venc_chn_attr.stRcAttr.stMjpegCbr.u32BitRate = u32Width * u32Height * 8;
		venc_chn_attr.stRcAttr.stMjpegCbr.u32BitRate = 70000000;//u32Width * u32Height * 8;
		//venc_chn_attr.stRcAttr.stMjpegCbr.u32BitRate = 50000000;//u32Width * u32Height * 8;

		venc_chn_attr.stVencAttr.imageType = IMAGE_TYPE_NV12;
		venc_chn_attr.stVencAttr.u32PicWidth = u32Width;
		venc_chn_attr.stVencAttr.u32PicHeight = u32Height;
		venc_chn_attr.stVencAttr.u32VirWidth = u32Width;
		venc_chn_attr.stVencAttr.u32VirHeight = u32Height;
		venc_chn_attr.stVencAttr.u32Profile = 77;
		ret = RK_MPI_VENC_CreateChn(chn, &venc_chn_attr);
		if (ret) {
			printf("ERROR: create VENC[1] error! ret=%d\n", ret);
			exit(1);
		}
	}


	return 0;
}

int init_vo(int w, int h, const char *u32PlaneType)
{
	int ret;
	int u32Fps = 60;
	int u32Zpos = 3;
	VO_CHN_ATTR_S stVoAttr = {0};
	stVoAttr.pcDevNode = "/dev/dri/card0";
	stVoAttr.u32Width = w;//u32DispWidth;//SCREEN_WIDTH;
	stVoAttr.u32Height = h;//u32DispHeight;//SCREEN_HEIGHT;
	stVoAttr.u16Fps = u32Fps;
	stVoAttr.u16Zpos = u32Zpos;
	stVoAttr.stImgRect.s32X = 0;
	stVoAttr.stImgRect.s32Y = 0;
	stVoAttr.stImgRect.u32Width = w;//u32DispWidth;
	stVoAttr.stImgRect.u32Height = h;//u32DispHeight;
	stVoAttr.stDispRect.s32X = 0;//s32Xpos;
	stVoAttr.stDispRect.s32Y = 0;//s32Ypos;
	stVoAttr.stDispRect.u32Width = w;//u32DispWidth;
	stVoAttr.stDispRect.u32Height = h;//u32DispHeight;

	stVoAttr.u16PlaneId = 89;
	stVoAttr.emPlaneType = VO_PLANE_OVERLAY;

	stVoAttr.u16ConIdx = 152;
	stVoAttr.u16EncIdx = 151;
	/*
	   if (!strcmp(u32PlaneType, "Primary")) {
	// VO[0] for primary plane
	stVoAttr.emPlaneType = VO_PLANE_PRIMARY;
	stVoAttr.enImgType = IMAGE_TYPE_RGB888;
	} else if (!strcmp(u32PlaneType, "Overlay")) {
	*/
	// VO[0] for overlay plane
	stVoAttr.emPlaneType = VO_PLANE_OVERLAY;
	stVoAttr.enImgType = IMAGE_TYPE_NV12;
	/*
	   } else {
	   printf("ERROR: Unsupport plane type:%s\n", u32PlaneType);
	   return -1;
	   }
	   */

	ret = RK_MPI_VO_CreateChn(0, &stVoAttr);
	if (ret) {
		printf("Create vo[0] failed! ret=%d\n", ret);
		exit(1);
	}
	return 0;
}

int deinit_vo()
{
	RK_MPI_VO_DestroyChn(0);
	return 0;
}


static RK_CHAR optstr[] = "?::a::x:y:d:H:W:w:h:r:I:M:";
static const struct option long_options[] = {
    {"vi_height", required_argument, NULL, 'H'},
    {"vi_width", required_argument, NULL, 'W'},
    {"crop_height", required_argument, NULL, 'h'},
    {"crop_width", required_argument, NULL, 'w'},
    {"device_name", required_argument, NULL, 'd'},
    {"crop_x", required_argument, NULL, 'x'},
    {"crop_y", required_argument, NULL, 'y'},
    {"rotation", required_argument, NULL, 'r'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");

  printf("\t%s [-H 1920] "
         "[-W 1080] "
         "[-h 640] "
         "[-w 640] "
         "[-x 300] "
         "[-y 300] "
         "[-r 0] "
         "[-I 0] "
         "[-d rkispp_scale0] \n",
         name);
  printf("\t-H | --vi_height: VI height, Default:1080\n");
  printf("\t-W | --vi_width: VI width, Default:1920\n");
  printf("\t-h | --crop_height: crop_height, Default:640\n");
  printf("\t-w | --crop_width: crop_width, Default:640\n");
  printf("\t-x  | --crop_x: start x of cropping, Default:300\n");
  printf("\t-y  | --crop_y: start y of cropping, Default:300\n");
  printf("\t-r  | --rotation, option[0, 90, 180, 270], Default:0\n");
  printf("\t-I | --camid: camera ctx id, Default 0\n");
  printf("\t-d  | --device_name set pcDeviceName, Default:/dev/video5\n");
  printf("\t  option: [rkispp_scale0, rkispp_scale1, rkispp_scale2]\n");
}

int main(int argc, char *argv[]) {
  int ret = 0;
  rga_demo_arg_t demo_arg;
  memset(&demo_arg, 0, sizeof(rga_demo_arg_t));
  demo_arg.target_width = 1280;
  demo_arg.target_height = 720;
  demo_arg.vi_width = 1920;
  demo_arg.vi_height = 1080;
  demo_arg.target_x = 0;
  demo_arg.target_y = 0;
  RK_S32 S32Rotation = 0;
  const char *device_name = "/dev/video5";
  RK_S32 s32CamId = 0;
  int c = 0;
  opterr = 1;
  while ((c = getopt_long(argc, argv, optstr, long_options, NULL)) != -1) {
    const char *tmp_optarg = optarg;
    switch (c) {
    case 'a':
      if (!optarg && NULL != argv[optind] && '-' != argv[optind][0]) {
        tmp_optarg = argv[optind++];
      }
      if (tmp_optarg) {
      } else {
      }
      break;
    case 'H':
      demo_arg.vi_height = atoi(optarg);
      break;
    case 'W':
      demo_arg.vi_width = atoi(optarg);
      break;
    case 'h':
      demo_arg.target_height = atoi(optarg);
      break;
    case 'w':
      demo_arg.target_width = atoi(optarg);
      break;
    case 'x':
      demo_arg.target_x = atoi(optarg);
      break;
    case 'y':
      demo_arg.target_y = atoi(optarg);
      break;
    case 'd':
      device_name = optarg;
      break;
    case 'r':
      S32Rotation = atoi(optarg);
      break;
    case 'I':
      s32CamId = atoi(optarg);
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf("device_name: %s\n\n", device_name);
  printf("#vi_height: %d\n\n", demo_arg.vi_height);
  printf("#vi_width: %d\n\n", demo_arg.vi_width);
  printf("#crop_x: %d\n\n", demo_arg.target_x);
  printf("#crop_y: %d\n\n", demo_arg.target_y);
  printf("#crop_height: %d\n\n", demo_arg.target_height);
  printf("#crop_width: %d\n\n", demo_arg.target_width);
  printf("#rotation: %d\n\n", S32Rotation);
  printf("#CameraIdx: %d\n\n", s32CamId);

  if (demo_arg.vi_height < (demo_arg.target_height + demo_arg.target_y) ||
      demo_arg.vi_width < (demo_arg.target_width + demo_arg.target_x)) {
    printf("crop size is over vi\n");
    return -1;
  }
  signal(SIGINT, sigterm_handler);

  RK_MPI_SYS_Init();

// Create buffer pool. Note that VO only support dma buffer.
  MB_POOL_PARAM_S stBufferPoolParam;
  stBufferPoolParam.u32Cnt = 10;
  stBufferPoolParam.u32Size =
      0; // Automatic calculation using imgInfo internally
  stBufferPoolParam.enMediaType = MB_TYPE_VIDEO;
  stBufferPoolParam.bHardWare = RK_TRUE;
  stBufferPoolParam.u16Flag = MB_FLAG_NOCACHED;
  stBufferPoolParam.stImageInfo.enImgType = IMAGE_TYPE_NV12;//stVoAttr.enImgType;
  stBufferPoolParam.stImageInfo.u32Width = SCREEN_WIDTH;
  stBufferPoolParam.stImageInfo.u32Height = SCREEN_HEIGHT;
  stBufferPoolParam.stImageInfo.u32HorStride = SCREEN_WIDTH;
  stBufferPoolParam.stImageInfo.u32VerStride = SCREEN_HEIGHT;

  mbp = RK_MPI_MB_POOL_Create(&stBufferPoolParam);
  if (!mbp) {
    printf("===========> error Create buffer pool for vo failed!\n");
	exit(1);
  }

  int w=1920;
  int h = 1080;
  const char *u32PlaneType = "Overlay";
  init_vo(w, h, u32PlaneType);






  g_stViChn.enModId = RK_ID_VI;
  g_stViChn.s32DevId = s32CamId;
  g_stViChn.s32ChnId = 1;
  g_stVencChn.enModId = RK_ID_VENC;
  g_stVencChn.s32DevId = 0;
  g_stVencChn.s32ChnId = 0;

  VI_CHN_ATTR_S vi_chn_attr;
  vi_chn_attr.pcVideoNode = device_name;
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = demo_arg.vi_width;
  vi_chn_attr.u32Height = demo_arg.vi_height;
  vi_chn_attr.enPixFmt = g_enPixFmt;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(g_stViChn.s32DevId, g_stViChn.s32ChnId,
                             &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(g_stViChn.s32DevId, g_stViChn.s32ChnId);
  if (ret) {
    printf("ERROR: Create vi[0] failed! ret=%d\n", ret);
    return -1;
  }

#if 0
  VENC_CHN_ATTR_S venc_chn_attr;
  memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
  venc_chn_attr.stVencAttr.enType = RK_CODEC_TYPE_H264;
  venc_chn_attr.stVencAttr.imageType = g_enPixFmt;
  venc_chn_attr.stVencAttr.u32PicWidth = demo_arg.vi_width;
  venc_chn_attr.stVencAttr.u32PicHeight = demo_arg.vi_height;
  venc_chn_attr.stVencAttr.u32VirWidth = demo_arg.vi_width;
  venc_chn_attr.stVencAttr.u32VirHeight = demo_arg.vi_height;
  venc_chn_attr.stVencAttr.u32Profile = 77;
  venc_chn_attr.stVencAttr.enRotation = (VENC_ROTATION_E)S32Rotation;

  venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;

  venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = 30;
  venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate =
      demo_arg.vi_width * demo_arg.vi_height * 30 / 14;
  venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen = 0;
  venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum = 30;
  venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen = 0;
  venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum = 30;

  RK_MPI_VENC_CreateChn(g_stVencChn.s32ChnId, &venc_chn_attr);
#endif
  init_venc();

  // venc
  pthread_t read_thread;
  pthread_create(&read_thread, NULL, GetMediaBuffer, &demo_arg);

  pthread_t venc_thread;
  {
	  char *output_file = NULL;
	  //const char *output_file = "out.h264";
	  pthread_create(&venc_thread, NULL, GetVencBuffer, (void *)output_file);
  }

  pthread_t read_venc1_thread;
  {
	  //const char *output_file = "test.jpeg";
	  const char *output_file = NULL;
	  pthread_create(&read_venc1_thread, NULL, GetJpgMediaBuffer, (void *)output_file);
  }

  usleep(1000); // waite for thread ready.
  ret = RK_MPI_VI_StartStream(g_stViChn.s32DevId, g_stViChn.s32ChnId);
  if (ret) {
    printf("ERROR: Start Vi[0] failed! ret=%d\n", ret);
    return -1;
  }

  printf("%s initial finish\n", __func__);
  while (!quit) {
    usleep(500000);
  }

  printf("%s exit!\n", __func__);
  pthread_join(read_thread, NULL);
  pthread_join(read_venc1_thread, NULL);
  pthread_join(venc_thread, NULL);

  //RK_MPI_VENC_DestroyChn(g_stVencChn.s32ChnId);
  deinit_venc();
  RK_MPI_VO_DestroyChn(0);
  printf("vichn=%d %d\n", g_stViChn.s32DevId, g_stViChn.s32ChnId);
  RK_MPI_VI_DisableChn(g_stViChn.s32DevId, g_stViChn.s32ChnId);

  RK_MPI_MB_POOL_Destroy(mbp);

  return 0;
}
