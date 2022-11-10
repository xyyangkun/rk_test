/******************************************************************************
 *
 *       Filename:  librkmedia_vi_get_frame.c
 *
 *    Description:  vi get yuv frame for qt
 *
 *        Version:  1.0
 *        Created:  2022年11月10日 14时28分46秒
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
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "rkmedia_api.h"
#include "librkmedia_vi_get_frame.h"

static OutYuvCb _cb = NULL;

void yuv_packet_cb(MEDIA_BUFFER mb) { 
#if 0
	printf("Get NV12 packet[%d]:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
			"timestamp:%lld\n",
			index_raw, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb),
			RK_MPI_MB_GetSize(mb), RK_MPI_MB_GetModeID(mb),
			RK_MPI_MB_GetChannelID(mb), RK_MPI_MB_GetTimestamp(mb));
	char nv12_path[64];
	sprintf(nv12_path, "/tmp/test_nv12_%d.yuv", index_raw++);
	FILE *file = fopen(nv12_path, "w");
	if (file) {
		fwrite(RK_MPI_MB_GetPtr(mb), 1, RK_MPI_MB_GetSize(mb), file); 
		fclose(file);
	}
#endif

	if(_cb)
	{
		_cb(RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb));
	}

	RK_MPI_MB_ReleaseBuffer(mb);
}

int init_vi(OutYuvCb cb)
{
	
	int ret;
	_cb = cb;
	int w = 1920;
	int h = 1080;

	RK_MPI_SYS_Init();
	VI_CHN_ATTR_S vi_chn_attr;
	vi_chn_attr.pcVideoNode = "/dev/video5";
	vi_chn_attr.u32BufCnt = 3;
	vi_chn_attr.u32Width = w;
	vi_chn_attr.u32Height = h;
	vi_chn_attr.enPixFmt = IMAGE_TYPE_NV12;
	vi_chn_attr.enWorkMode = VI_WORK_MODE_NORMAL;
	vi_chn_attr.enBufType = VI_CHN_BUF_TYPE_MMAP;
	ret = RK_MPI_VI_SetChnAttr(0, 0, &vi_chn_attr);
	ret |= RK_MPI_VI_EnableChn(0, 0);
	if (ret) {
		printf("Create VI[0] failed! ret=%d\n", ret);
		return -1;
	}

	printf("%s initial finish\n", __func__);

	MPP_CHN_S stViChn;
	stViChn.enModId = RK_ID_VI;
	stViChn.s32ChnId = 0;
	ret = RK_MPI_SYS_RegisterOutCb(&stViChn, yuv_packet_cb);
	if (ret) 
	{
		printf("Register Output callback failed! ret=%d\n", ret);
		return -1;
	}
	ret = RK_MPI_VI_StartStream(0, 0);
	if (ret) 
	{
		printf("Start VI[0] failed! ret=%d\n", ret);
		return -1;
	}

	return 0;
}

int deinit_vi()
{
  RK_MPI_VI_DisableChn(0, 0);
  return 0;
}
