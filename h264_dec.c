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
#include <stdlib.h>
#include <assert.h>
#include "h264_dec.h"
#include "rk_h264_decode.h"
#include "h264_annexb_nalu.h"
#include "easymedia/rkmedia_api.h"
#include "myutils.h"

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

typedef struct s_h264_dec
{
	FILE *fp_yuv;
	FILE *fp_h264_out;
	struct vpu_h264_decode dec;
	MEDIA_BUFFER_POOL mbp;
}t_h264_dec;

// 获得h264 帧，可以进行解码
static void h264_handler(void* param, const uint8_t* nalu, size_t bytes)
{
	int ret;
	printf("===============>yk debug!, param=%p==>nalu %d== %02x %02x %02x %02x\n", param, bytes, nalu[0], nalu[1], nalu[2], nalu[3]);
	t_h264_dec *dec = (t_h264_dec*)param;

	// 写入解析出来的h264帧
	if(dec->fp_h264_out)fwrite(nalu, bytes, 1, dec->fp_h264_out);



	// 进行解码
	int dtype = 0;
	MEDIA_BUFFER mb;
	MEDIA_BUFFER mb_vpu = NULL;
	mb_vpu = RK_MPI_MB_POOL_GetBuffer(dec->mbp, RK_TRUE);
	if (!mb_vpu) {
		printf("ERROR: BufferPool get null buffer...\n");
		exit(1);
	}
	mb = RK_MPI_MB_CreateBuffer(bytes, RK_FALSE, 0);
	if (!mb_vpu) {
		printf("ERROR: Buffer get null buffer...\n");
		exit(1);
	}
	memcpy(RK_MPI_MB_GetPtr(mb), nalu, bytes);
	printf("%s %d size===>%d\n", __FUNCTION__, __LINE__, RK_MPI_MB_GetSize(mb));
	ret = vpu_decode_h264_doing(&dec->dec, RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb),
			RK_MPI_MB_GetFD(mb_vpu), RK_MPI_MB_GetPtr(mb_vpu) , &dtype);
	if(ret !=0 )
	{
		printf("ERROR:decode!ret=%d\n", ret);
		exit(1);
	}
	printf("%s %d\n", __FUNCTION__, __LINE__);

	// 写入解码后数据
	//if(dec->fp_yuv)fwrite(RK_MPI_MB_GetPtr(mb_vpu), RK_MPI_MB_GetSize(mb_vpu), 1, dec->fp_yuv);

	// 释放数据
	RK_MPI_MB_ReleaseBuffer(mb);
	RK_MPI_MB_ReleaseBuffer(mb_vpu);
}

static int is_start = 1;
static pthread_t thread_dec;
static void *test_h264_dec_proc(void *param)
{
	char *h264 = "./tennis200.h264";
	//char *h264 = "./1080P.h265";
	
	FILE* fp_h264_out = fopen("out.h264", "wb+");
	FILE* fp_yuv = fopen("out.yuv", "wb+");
	int ret;

	t_h264_dec dec;
	memset(&dec, 0, sizeof(t_h264_dec));
	int w=1920;
	int h = 1080;
	// 创建解码器
	ret = vpu_decode_h264_init(&dec.dec, w, h);
	if(ret != 0)
	{
		printf("error in h264 dec init,ret=%d\n", ret);
		exit(0);
	}
	dec.fp_h264_out = fp_h264_out;
	dec.fp_yuv = fp_yuv;
	dec.dec.fp_output = fp_yuv;

	// 创建内存池
	MB_POOL_PARAM_S stBufferPoolParam;
	stBufferPoolParam.u32Cnt = 5;
	stBufferPoolParam.u32Size =
		0; // Automatic calculation using imgInfo internally
	stBufferPoolParam.enMediaType = MB_TYPE_VIDEO;
	stBufferPoolParam.bHardWare = RK_TRUE;
	stBufferPoolParam.u16Flag = MB_FLAG_NOCACHED;
	stBufferPoolParam.stImageInfo.enImgType = IMAGE_TYPE_NV12;
	stBufferPoolParam.stImageInfo.u32Width = w;
	stBufferPoolParam.stImageInfo.u32Height = h;
	stBufferPoolParam.stImageInfo.u32HorStride = ALIGN16(w);
	stBufferPoolParam.stImageInfo.u32VerStride = ALIGN16(h);
	dec.mbp = RK_MPI_MB_POOL_Create(&stBufferPoolParam);
	if (!dec.mbp) {
		printf("ERROR in Create usb video buffer pool for vo failed! w:%d h:%d\n", w, h);
		exit(1);
	}

	// 读取h264文件
	long bytes = 0;
	uint8_t* ptr = file_read(h264, &bytes);
	while(is_start == 1) {
		uint8_t *p = ptr;
		// 解析h264文件
		ret = mpeg4_h264_annexb_nalu(p, bytes, h264_handler, (void*)&dec);
		if(ret == 0)break;

	}

	ret= vpu_decode_h264_done(&dec.dec);
	assert(ret==0);

	if(fp_h264_out){
		fclose(fp_h264_out);
	}
	if(fp_yuv){
		fclose(fp_yuv);
	}

	// 删除内存池
	RK_MPI_MB_POOL_Destroy(dec.mbp);

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
