/******************************************************************************
 *
 *       Filename:  h264_rkmedia_dec.c
 *
 *    Description:  test rkmeda h264 dec
 *
 *        Version:  1.0
 *        Created:  2022年11月09日 13时19分51秒
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
#include "h264_rkmedia_dec.h"
#include "h264_annexb_nalu.h"
#include "easymedia/rkmedia_api.h"
#include "myutils.h"

// 解码后调用显示
#define VDEC_DISPLAY

static int is_start = 1;
static pthread_t thread_dec;

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

typedef struct s_h264_rkmedia_dec
{
	FILE *fp_yuv;
	FILE *fp_h264_out;
	MEDIA_BUFFER_POOL mbp;
}t_h264_rkmedia_dec;

// 获得h264 帧，可以进行解码
static void h264_handler(void* param, const uint8_t* nalu, size_t bytes)
{
	int ret;
	printf("===============>yk debug!, param=%p==>nalu %lu== %02x %02x %02x %02x\n", param, bytes, nalu[0], nalu[1], nalu[2], nalu[3]);
	print_fps("==================> send h264=");
	t_h264_rkmedia_dec *dec = (t_h264_rkmedia_dec*)param;

	// 写入解析出来的h264帧
	if(dec->fp_h264_out)fwrite(nalu, bytes, 1, dec->fp_h264_out);



	// 进行解码
	MEDIA_BUFFER mb_vpu = NULL;
	mb_vpu = RK_MPI_MB_POOL_GetBuffer(dec->mbp, RK_TRUE);
	if (!mb_vpu) {
		printf("ERROR: BufferPool get null buffer...\n");
		exit(1);
	}
	memcpy(RK_MPI_MB_GetPtr(mb_vpu), nalu, bytes);
	RK_MPI_MB_SetSize(mb_vpu, bytes);
	printf("%s %d size===>%lu\n", __FUNCTION__, __LINE__, RK_MPI_MB_GetSize(mb_vpu));

	ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_VDEC, 0, mb_vpu);
	if(ret !=0 )
	{
		printf("ERROR:decode!ret=%d\n", ret);
		exit(1);
	}
	printf("%s %d\n", __FUNCTION__, __LINE__);
	//usleep(5*1000);
	usleep(20*1000);


	// 写入解码后数据
	//if(dec->fp_yuv)fwrite(RK_MPI_MB_GetPtr(mb_vpu), RK_MPI_MB_GetSize(mb_vpu), 1, dec->fp_yuv);

	// 释放数据
	RK_MPI_MB_ReleaseBuffer(mb_vpu);
}

static void *GetMediaBuffer(void *arg) 
{ 
	t_h264_rkmedia_dec *dec = (t_h264_rkmedia_dec*)arg;
	MEDIA_BUFFER mb = NULL;

	int ret = 0;
	while(is_start==1)
	{
		mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_VDEC, 0, 5000);
		if (!mb)
		{
			if(is_start != 1)
			break;
			printf("RK_MPI_SYS_GetMediaBuffer get null buffer in 5s...\n");
			//return NULL;
			continue; 
		}
		MB_IMAGE_INFO_S stImageInfo = {0};                                           
		ret = RK_MPI_MB_GetImageInfo(mb, &stImageInfo);                              
		if (ret) {                                                                   
			printf("Get image info failed! ret = %d\n", ret);                        
			RK_MPI_MB_ReleaseBuffer(mb);                                             
			return NULL;                                                             
		}                                                                            
	print_fps("==================> recv h264=");

#if 1
		printf(">>>>Get Frame:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "            
				"timestamp:%lld, ImgInfo:<wxh %dx%d, fmt 0x%x>\n",                   
				RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetFD(mb), RK_MPI_MB_GetSize(mb), 
				RK_MPI_MB_GetModeID(mb), RK_MPI_MB_GetChannelID(mb),                 
				RK_MPI_MB_GetTimestamp(mb), stImageInfo.u32Width,                    
				stImageInfo.u32Height, stImageInfo.enImgType);  
#endif
		// 写入解码后数据
		if(dec->fp_yuv)
		{
			//fwrite(RK_MPI_MB_GetPtr(mb), RK_MPI_MB_GetSize(mb), 1, dec->fp_yuv);
		}
#ifdef VDEC_DISPLAY
			// 送显
			ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_VO, 0, mb);
			if(ret != 0)
			{
				printf("error to send _mb to vo display!, will exit\n");
				exit(0);
			}

#endif
	

		RK_MPI_MB_ReleaseBuffer(mb);
	}
}

static void *test_h264_dec_proc(void *param)
{
	//char *h264 = "./tennis200.h264";
	char *h264 = "../rk3568_1080p.h264";
	//char *h264 = "./1080P.h265";
	
	printf("will open h264 file:%s\n", h264);
	FILE* fp_h264_out = fopen("out.h264", "wb+");
	FILE* fp_yuv = fopen("out.yuv", "wb+");
	int ret;

	t_h264_rkmedia_dec dec;
	memset(&dec, 0, sizeof(t_h264_rkmedia_dec));
	int w=1920;
	int h = 1080;

	dec.fp_h264_out = fp_h264_out;
	dec.fp_yuv = fp_yuv;

	RK_MPI_SYS_Init();	
	// 创建rkmedia解码器
	LOG_LEVEL_CONF_S conf = {RK_ID_VDEC, 5, "all"};
	ret = RK_MPI_LOG_SetLevelConf(&conf);
	if(ret!=0)
	{
		printf("error in set levelconf!\n");
		exit(1);
	}
	printf("===============================> set log level!\n");


	VDEC_CHN_ATTR_S stVdecAttr={0};
	stVdecAttr.enCodecType = RK_CODEC_TYPE_H264;
	//stVdecAttr.enMode = VIDEO_MODE_FRAME;
	// 很奇怪,要使用stream模式才能正确解码
	stVdecAttr.enMode = VIDEO_MODE_STREAM;
	stVdecAttr.enDecodecMode = VIDEO_DECODEC_HADRWARE;
	ret = RK_MPI_VDEC_CreateChn(0, &stVdecAttr);
	if(ret)
	{
		 printf("Create Vdec[0] failed! ret=%d\n", ret);
		 exit(1);
	}


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

	// 创建接收解码后数据线程
	pthread_t read_thread;
	pthread_create(&read_thread, NULL, GetMediaBuffer, (void*)&dec);

	// 读取h264文件
	long bytes = 0;
	uint8_t* ptr = file_read(h264, &bytes);
	while(is_start == 1) {
		uint8_t *p = ptr;
		// 解析h264文件
		ret = mpeg4_h264_annexb_nalu(p, bytes, h264_handler, (void*)&dec);
		if(ret == 0){
			if(is_start != 1)
			break;
		}

	}

	printf("will exit thread================================>!!!\n");
	is_start = 0;

	pthread_join(read_thread, NULL);

	if(fp_h264_out){
		fclose(fp_h264_out);
	}
	if(fp_yuv){
		fclose(fp_yuv);
	}



	RK_MPI_VDEC_DestroyChn(0);

	// 删除内存池
	RK_MPI_MB_POOL_Destroy(dec.mbp);

	free(ptr);
	return NULL;
}

void stop_test_h264_rkmedia_dec()
{
	is_start = 0;
	pthread_join(thread_dec, NULL);

#ifdef VDEC_DISPLAY
	// 释放解码显示
	deinit_dec_vo();
#endif

}

void start_test_h264_rkmedia_dec()
{

#ifdef VDEC_DISPLAY
	// 初始化解码显示
	init_dec_vo(1920, 1080);
	//init_dec_vo(1280, 720); // 720p vo
#endif

	is_start = 1;
	int ret = pthread_create(&thread_dec, NULL, test_h264_dec_proc, NULL);	
	if(ret!=0 )
	{
		printf("error to create thread!\n");
		exit(0);
	}
}
