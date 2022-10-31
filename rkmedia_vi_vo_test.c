// Copyright 2020 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

//#include "common/sample_common.h"
#include "rkmedia_api.h"

static bool quit = false;
static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
}

static RK_CHAR optstr[] = "?::a::I:M:";
static const struct option long_options[] = {
    {"aiq", optional_argument, NULL, 'a'},
    {"camid", required_argument, NULL, 'I'},
    {"multictx", required_argument, NULL, 'M'},
    {"help", optional_argument, NULL, '?'},
    {NULL, 0, NULL, 0},
};

static void print_usage(const RK_CHAR *name) {
  printf("usage example:\n");

  printf("\t%s [-I 0]\n", name);
  printf("\t-I | --camid: camera ctx id, Default 0\n");
}

int main(int argc, char *argv[]) {
  int ret = 0;
  int video_width = 1920;
  int video_height = 1080;
  int disp_width = 1080;//720;
  int disp_height = 1920;//1280;
  RK_S32 s32CamId = 0;
  int c;
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
    case 'I':
      s32CamId = atoi(optarg);
      break;
    case '?':
    default:
      print_usage(argv[0]);
      return 0;
    }
  }

  printf("#CameraIdx: %d\n\n", s32CamId);

  RK_MPI_SYS_Init();
  VI_CHN_ATTR_S vi_chn_attr;
  //vi_chn_attr.pcVideoNode = "rkispp_scale0";
  vi_chn_attr.pcVideoNode = "/dev/video5";
  vi_chn_attr.u32BufCnt = 3;
  vi_chn_attr.u32Width = video_width;
  vi_chn_attr.u32Height = video_height;
  vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
  vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
  ret = RK_MPI_VI_SetChnAttr(s32CamId, 0, &vi_chn_attr);
  ret |= RK_MPI_VI_EnableChn(s32CamId, 0);
  if (ret) {
    printf("Create vi[0] failed! ret=%d\n", ret);
    return -1;
  }

#if 0
  /* test rgn cover */
  COVER_INFO_S CoverInfo;
  OSD_REGION_INFO_S RngInfo;
  memset(&CoverInfo, 0, sizeof(CoverInfo));
  memset(&RngInfo, 0, sizeof(RngInfo));
  CoverInfo.enPixelFormat = PIXEL_FORMAT_ARGB_8888;
  CoverInfo.u32Color = 0xFFFF0000; // blue
  RngInfo.enRegionId = REGION_ID_0;
  RngInfo.u32PosX = 0;
  RngInfo.u32PosY = 0;
  RngInfo.u32Width = 100;
  RngInfo.u32Height = 100;
  RngInfo.u8Enable = 1;
  RK_MPI_VI_RGN_SetCover(s32CamId, 0, &RngInfo, &CoverInfo);
#endif

  // rga0 for primary plane
  RGA_ATTR_S stRgaAttr;
  memset(&stRgaAttr, 0, sizeof(stRgaAttr));
  stRgaAttr.bEnBufPool = RK_TRUE;
  //stRgaAttr.u16BufPoolCnt = 3;
  stRgaAttr.u16BufPoolCnt = 5;
  //stRgaAttr.u16Rotaion = 90;
  stRgaAttr.u16Rotaion = 270;
  stRgaAttr.stImgIn.u32X = 0;
  stRgaAttr.stImgIn.u32Y = 0;
  stRgaAttr.stImgIn.imgType = IMAGE_TYPE_NV12;
  stRgaAttr.stImgIn.u32Width = video_width;
  stRgaAttr.stImgIn.u32Height = video_height;
  stRgaAttr.stImgIn.u32HorStride = video_width;
  stRgaAttr.stImgIn.u32VirStride = video_height;
  stRgaAttr.stImgOut.u32X = 0;
  stRgaAttr.stImgOut.u32Y = 0;
  stRgaAttr.stImgOut.imgType = IMAGE_TYPE_NV12;//IMAGE_TYPE_RGB888;
  stRgaAttr.stImgOut.u32Width = disp_width;
  stRgaAttr.stImgOut.u32Height = disp_height;
  stRgaAttr.stImgOut.u32HorStride = disp_width;
  stRgaAttr.stImgOut.u32VirStride = disp_height;
  ret = RK_MPI_RGA_CreateChn(0, &stRgaAttr);
  if (ret) {
    printf("Create rga[0] falied! ret=%d\n", ret);
    return -1;
  }

  VO_CHN_ATTR_S stVoAttr = {0};
  // VO[0] for primary plane
  stVoAttr.pcDevNode = "/dev/dri/card0";
  stVoAttr.emPlaneType = VO_PLANE_PRIMARY;
  stVoAttr.enImgType = IMAGE_TYPE_NV12;//IMAGE_TYPE_RGB888;
  stVoAttr.u16Zpos = 0;
  stVoAttr.stDispRect.s32X = 0;
  stVoAttr.stDispRect.s32Y = 0;
  stVoAttr.stDispRect.u32Width = disp_width;
  stVoAttr.stDispRect.u32Height = disp_height;

  stVoAttr.u16PlaneId = 103;
  stVoAttr.emPlaneType = VO_PLANE_OVERLAY;
  stVoAttr.u16ConIdx = 163;
  stVoAttr.u16EncIdx = 162;

  ret = RK_MPI_VO_CreateChn(0, &stVoAttr);
  if (ret) {
    printf("Create vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  MPP_CHN_S stSrcChn = {0};
  MPP_CHN_S stDestChn = {0};

  printf("#Bind VI[0] to RGA[0]....\n");
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind vi[0] to rga[0] failed! ret=%d\n", ret);
    return -1;
  }

  printf("# Bind RGA[0] to VO[0]....\n");
  stSrcChn.enModId = RK_ID_RGA;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("Bind rga[0] to vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  printf("%s initial finish\n", __func__);
  signal(SIGINT, sigterm_handler);
  while (!quit) {
    usleep(500000);
  }

  printf("%s exit!\n", __func__);
  stSrcChn.enModId = RK_ID_VI;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_RGA;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind vi[0] to rga[0] failed! ret=%d\n", ret);
    return -1;
  }

  stSrcChn.enModId = RK_ID_RGA;
  stSrcChn.s32ChnId = 0;
  stDestChn.enModId = RK_ID_VO;
  stDestChn.s32ChnId = 0;
  ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
  if (ret) {
    printf("UnBind rga[0] to vo[0] failed! ret=%d\n", ret);
    return -1;
  }

  RK_MPI_VO_DestroyChn(0);
  RK_MPI_RGA_DestroyChn(0);
  RK_MPI_VI_DisableChn(s32CamId, 0);

  return 0;
}
