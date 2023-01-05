/******************************************************************************
 *
 *       Filename:  test_dec_enc.cpp
 *
 *    Description:  测试解码后数据进行编码
 *    				1. 分辨率，帧率更改
 *    				2. 码率更改
 *    				3. I帧间隔更改
 *    		4. 主要rkmedia解码，解码后的mb数据，进行编码, 
 *    		5. 编码后数据，最多可以缓存多少帧？
 *    		6. 编码时间戳
 *
 *        Version:  1.0
 *        Created:  2022年12月16日 15时55分57秒
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
#include <unistd.h>
#include "easymedia/rkmedia_api.h"
#include "myutils.h"
#include "h264_file_parse.h"

// rkmpp 编码
#include "RkMppEncoder.h"
#ifdef ZLOG
#include "zlog_api.h"
zlog_category_t * log_category = NULL;
#define zlog_path "zlog.conf" 
#endif

static bool quit = false;

static int is_start = 1;
static pthread_t thread_dec;

typedef struct s_h264_rkmedia_dec
{
	FILE *fp_yuv;
	FILE *fp_h264_out;
	MEDIA_BUFFER_POOL mbp;

    osee::MppEncoder mppenc;
	unsigned int w, h;
	FILE *enc_h264;
}t_h264_rkmedia_dec;

// 获得h264 帧，可以进行解码
static void h264_handler(void* param, const uint8_t* nalu, size_t bytes)
{
	int ret;
	log_print("===============>yk debug!, param=%p==>nalu %lu== %02x %02x %02x %02x\n", param, bytes, nalu[0], nalu[1], nalu[2], nalu[3]);
	//print_fps("==================> send h264=");
	t_h264_rkmedia_dec *dec = (t_h264_rkmedia_dec*)param;

	// 写入解析出来的h264帧
	if(dec->fp_h264_out)fwrite(nalu, bytes, 1, dec->fp_h264_out);



	// 进行解码
	MEDIA_BUFFER mb_vpu = NULL;
	mb_vpu = RK_MPI_MB_POOL_GetBuffer(dec->mbp, RK_TRUE);
	if (!mb_vpu) {
		log_error("ERROR: BufferPool get null buffer...\n");
		exit(1);
	}
	memcpy(RK_MPI_MB_GetPtr(mb_vpu), nalu, bytes);
	RK_MPI_MB_SetSize(mb_vpu, bytes);
	log_print("%s %d size===>%lu\n", __FUNCTION__, __LINE__, RK_MPI_MB_GetSize(mb_vpu));

	ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_VDEC, 0, mb_vpu);
	if(ret !=0 )
	{
		log_print("ERROR:decode!ret=%d\n", ret);
		exit(1);
	}
	log_print("%s %d\n", __FUNCTION__, __LINE__);
	//usleep(5*1000);
	//usleep(10*1000);
	usleep(15*1000);
	//usleep(35*1000);


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
			log_print("RK_MPI_SYS_GetMediaBuffer get null buffer in 5s...\n");
			//return NULL;
			continue; 
		}
		MB_IMAGE_INFO_S stImageInfo = {0};                                           
		ret = RK_MPI_MB_GetImageInfo(mb, &stImageInfo);                              
		if (ret) {                                                                   
			log_print("Get image info failed! ret = %d\n", ret);                        
			RK_MPI_MB_ReleaseBuffer(mb);                                             
			return NULL;                                                             
		}                                                                            
	print_fps("==================>from h264 file recv h264 =");

#if 1
		log_print("Get Frame:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "            
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

		if(stImageInfo.u32Width != dec->w || stImageInfo.u32Height != dec->h)
		{
			dec->w = stImageInfo.u32Width;
			dec->h = stImageInfo.u32Height;
			printf("===========> resolution change : %d %d\n", dec->w, dec->h);

			dec->mppenc.set_resolution(dec->w, dec->h, ALIGN16(dec->w), ALIGN16(dec->h));
		}
		
		// 进行编码
		//char dst[1024*1024*4];
		//int length = 0;
		MEDIA_BUFFER _mb = NULL;
		_mb = RK_MPI_MB_POOL_GetBuffer(dec->mbp, RK_TRUE);
		if (!_mb) {
			log_error("ERROR: BufferPool get null buffer...\n");
			exit(1);
		}
        dec->mppenc.encode((void*)mb, _mb);
		
		// 写入编码后额h264数据
		if(dec->enc_h264)
		{
			fwrite(RK_MPI_MB_GetPtr(_mb), RK_MPI_MB_GetSize(_mb), 1, dec->enc_h264);
		}
		

		RK_MPI_MB_ReleaseBuffer(mb);
		RK_MPI_MB_ReleaseBuffer(_mb);
	}
	return NULL;
}

t_h264_rkmedia_dec dec;

tyk_h264_parse parse = {0};
// rkmedia解码后 mpp在进行编码
static void *test_h264_dec_proc(void *param)
{
	const char *h264_1080p = "./tennis200.h264";
	//char *h264 = "../rk3568_1080p.h264";
	//char *h264 = "/mnt/sdcard/out_120s.h264";
	//char *h264 = "./1080P.h265";
	//const char *h264_720p = "../rk3568_720p.h264";
	const char *h264_720p = "../test_720p.h264";
	(void)h264_720p;
	
	FILE* fp_h264_out = fopen("out.h264", "wb+");
	FILE* fp_yuv = fopen("out.yuv", "wb+");
	int ret;

	memset((void*)&dec, 0, sizeof(t_h264_rkmedia_dec));
	int w=1920;
	int h = 1080;

	//dec.fp_h264_out = fp_h264_out;
	dec.fp_yuv = fp_yuv;

	// init venc
	dec.mppenc.MppEncdoerInit(1920, 1080, 60);
	dec.enc_h264 = fopen("enc.h264", "wb+");



	RK_MPI_SYS_Init();	
#if 0
	// 创建rkmedia解码器
	LOG_LEVEL_CONF_S conf = {RK_ID_VDEC, 5, "all"};
	ret = RK_MPI_LOG_SetLevelConf(&conf);
	if(ret!=0)
	{
		printf("error in set levelconf!\n");
		exit(1);
	}
	log_print("===============================> set log level!\n");
#endif


	VDEC_CHN_ATTR_S stVdecAttr;
	stVdecAttr.enCodecType = RK_CODEC_TYPE_H264;
	stVdecAttr.enMode = VIDEO_MODE_FRAME;
	//stVdecAttr.enMode = VIDEO_MODE_STREAM;
	stVdecAttr.enDecodecMode = VIDEO_DECODEC_HADRWARE;
	ret = RK_MPI_VDEC_CreateChn(0, &stVdecAttr);
	if(ret)
	{
		 log_error("Create Vdec[0] failed! ret=%d\n", ret);
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
		log_error("ERROR in Create usb video buffer pool for vo failed! w:%d h:%d\n", w, h);
		exit(1);
	}

	// 创建接收解码后数据线程
	pthread_t read_thread;
	pthread_create(&read_thread, NULL, GetMediaBuffer, (void*)&dec);

	// 测试720p 1080p 切换编码
#define CHANGE_720_1080 1


	parse.is_run = 1;
	dec.w = 1920;
	dec.h = 1080;

	// 参数
	while(1)
	{
		if(parse.is_run == 0)break;
		{
			// 打开1080p文件
			strcpy(parse.file_name, h264_1080p);
			parse.mini_buf_size= 1024*1024;
#if CHANGE_720_1080==1
			parse.bloop = 0; // 之编码一次
#else
			parse.bloop = 1;
#endif
			parse.handler = h264_handler;
			parse.param = &dec;


			ret = yk_h264_parse(&parse);
			if(ret != 0)
			{
				log_error("ERROR in yk parse\n");
				exit(1);
			}
		}

		if(parse.is_run == 0)break;

#if CHANGE_720_1080==1
		// 测试720p 1080p文件切换解码编码
		{
			// 解码时改变
			//dec.w = 1280;
			//dec.h = 720;
			// 打开720p文件
			strcpy(parse.file_name, h264_720p);
			parse.mini_buf_size= 1024*1024;
			//parse.is_run = 1;
			parse.bloop = 0; // 之编码一次
			//parse.bloop = 1;
			parse.handler = h264_handler;
			parse.param = &dec;


			ret = yk_h264_parse(&parse);
			if(ret != 0)
			{
				log_error("ERROR in yk parse\n");
				exit(1);
			}
		}
		if(parse.is_run == 0)break;
#else
	break;
#endif



	}






	log_error("will exit thread================================>!!!\n");
	is_start = 0;

	pthread_join(read_thread, NULL);

	if(fp_h264_out){
		fclose(fp_h264_out);
	}
	if(fp_yuv){
		fclose(fp_yuv);
	}

	if(dec.enc_h264){
		fclose(dec.enc_h264);
	}



	RK_MPI_VDEC_DestroyChn(0);

	// 删除内存池
	RK_MPI_MB_POOL_Destroy(dec.mbp);

	//free(ptr);
	return NULL;
}

static void stop_test_h264_rkmedia_dec()
{
	is_start = 0;
	pthread_join(thread_dec, NULL);

}

static void start_test_h264_rkmedia_dec()
{
	is_start = 1;
	int ret = pthread_create(&thread_dec, NULL, test_h264_dec_proc, NULL);	
	if(ret!=0 )
	{
		log_error("error to create thread!\n");
		exit(0);
	}
}

static void sigterm_handler(int sig) {
  fprintf(stderr, "signal %d\n", sig);
  quit = true;
  parse.is_run = 0;
}

void useage()
{
	printf("q exit\n");
	printf("1 change gop/fps to 30\n");
	printf("2 change gop/fps to 60\n");
	printf("3 change birtat 80m\n");
	printf("4 change birtat 10m\n");

}


int main(int argc, char *argv[])
{
#ifdef ZLOG
	if(zlog_init(zlog_path)) {
		printf("ERROR in zlog_init\n");
		zlog_fini();
		exit(1);
	}
	log_category = zlog_get_category("rktest");
	if(!log_category)
	{
		fprintf(stderr, "============================> Error in get comm protcol category\n");
		zlog_fini();
	}
#endif
	_dbg_init();

	start_test_h264_rkmedia_dec();

	log_print("%s initial finish\n", __func__);
	signal(SIGINT, sigterm_handler);

#if 1
	// 菜单
	while(1)
	{
		int ret;
		char ch;
		useage();
		scanf("%c", &ch);
		getchar();
		printf("you input, ch=%c\n", ch);
		switch(ch)
		{
			case 'q':
				goto exit;
			break;

			case '1':
				ret = dec.mppenc.set_fps(30);	
				if(ret != 0){printf("%d", __LINE__); exit(1);}
				ret = dec.mppenc.set_gop(30);	
				if(ret != 0){printf("%d", __LINE__); exit(1);}
			break;

			case '2':
				ret=dec.mppenc.set_fps(60);	
				if(ret != 0){printf("%d", __LINE__); exit(1);}
				ret=dec.mppenc.set_gop(60);	
				if(ret != 0){printf("%d", __LINE__); exit(1);}
			break;

			case '3':
				ret=dec.mppenc.set_bitrate(80000000);	
				if(ret != 0){printf("%d", __LINE__); exit(1);}
				
			break;

			case '4':
				ret=dec.mppenc.set_bitrate(10000000);	
				if(ret != 0){printf("%d", __LINE__); exit(1);}
			break;

			default:
			goto exit;
			break;
		}

	}
#endif

	while (!quit) {
		usleep(500000);
	}
exit:
	quit = true;
	parse.is_run = 0;

	log_print("will exit thread!");
	printf("will exit thread!");

	stop_test_h264_rkmedia_dec();


#ifdef ZLOG
	zlog_fini();
#endif
	
	return 0;
}

