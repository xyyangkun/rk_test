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

#include "mb_get.h"
#include "list.h"

#define MODULE "h264_dec"
#include "myutils.h"

// 解码后调用显示
#define VDEC_DISPLAY

// 开启时，进行1080P解码，取消时进行720p解码
//#define DEC_1080P

static int is_start = 1;

// 发送到vo进行显示
// ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_VO, 0, mb);
// if(ret != 0)RK_MPI_MB_ReleaseBuffer(mb); // 失败
// 初始化hdmi vo
int init_dec_vo(int w, int h)
{
	//const char *u32PlaneType = "Overlay";
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

int deinit_dec_vo()
{
  RK_MPI_VO_DestroyChn(0);
  return 0;
}



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
typedef struct mylist_s
{
	char str[40];
	void *hd;
	list_t list;
}mylist;

typedef struct s_h264_dec
{
	FILE *fp_yuv;
	FILE *fp_h264_out;
	struct vpu_h264_decode dec;
	MEDIA_BUFFER_POOL mbp;
	//list_t head;
}t_h264_dec;

static int _count = 0;

#if 0
// 解码后获取数据回调
static int h264_vdec_handle(void *param, void *mb)
{
	int ret;
	MEDIA_BUFFER _mb = (MEDIA_BUFFER)mb;;
	if(_mb == NULL)
	{
		printf("error in vdec handle null ptr!\n");
		exit(1);
	}

	// 使用mb，释放mb
	if(_mb)
	{

#ifdef VDEC_DISPLAY
		ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_VO, 0, _mb);
		if(ret != 0)
		{
			printf("error to send mb to vo display!, will exit\n");
			exit(0);
		}
#endif
		ret = RK_MPI_MB_ReleaseBuffer(_mb);
		//ret = RK_MPI_MB_release(_mb);
		if(ret != 0)
		{
			printf("error in release dec buff!\n");
			exit(1);
		}

	}

	return 0;
}
#endif


static unsigned long long _pts = 0;
#define _DIFF 20
// 获得h264 帧，可以进行解码
static int h264_handler(void* param, const uint8_t* nalu, size_t bytes)
{
	int mark = 0;
	int ret;
	dbg("===============>yk debug!, param=%p==>nalu %ld== %02x %02x %02x %02x %02x\n",
			param, bytes, nalu[0], nalu[1], nalu[2], nalu[3], nalu[4]);
	t_h264_dec *dec = (t_h264_dec*)param;

	if(is_start != 1)
	{
		dbg("will break!\n");
		return -1;
	}
	// 写入解析出来的h264帧
	if(dec->fp_h264_out)fwrite(nalu, bytes, 1, dec->fp_h264_out);

	// 进行解码
	MEDIA_BUFFER mb;
#if 0
	MEDIA_BUFFER mb_vpu = NULL;
	mb_vpu = RK_MPI_MB_POOL_GetBuffer(dec->mbp, RK_TRUE);
	if (!mb_vpu) {
		printf("ERROR: BufferPool get null buffer...\n");
		exit(1);
	}
#endif
	mb = RK_MPI_MB_CreateBuffer(bytes, RK_FALSE, 0);
	if (!mb) {
		printf("ERROR: Buffer get null buffer...\n");
		exit(1);
	}
	memcpy(RK_MPI_MB_GetPtr(mb), nalu, bytes);

	// 有些nalu 时间戳不应该累加
	unsigned char nalu_type = nalu[4]&0xff;
	if(0x67 != nalu_type && 0x68 != nalu_type)
	{
		_pts += _DIFF;
	}
	dbg("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< h264 pts:%llu\n", _pts);
	RK_MPI_MB_SetTimestamp(mb, _pts);

	dbg("%s %d size===>%lu\n", __FUNCTION__, __LINE__, RK_MPI_MB_GetSize(mb));

	int size;
	void *mb_array[10];
	ret = vpu_decode_h264_doing(&dec->dec, mb,
			&mb_array[0], &size);
			
	if(ret !=0 )
	{
		printf("ERROR:decode!ret=%d\n", ret);
		exit(1);
	}

	// 解码出数据
	if(size > 0)
	{
		// 遍历解码后的数据，进行处理
		for(int i=0; i<size; i++)
		{
			_count ++;
			MEDIA_BUFFER _mb = (MEDIA_BUFFER)mb_array[i];
			if(_mb == NULL)
			{
				dbg("error in get _mb null!, will exit\n");
				exit(1);
			}
		
#if 0
			// 分配内存
			mylist *li;
			li = (mylist *)malloc(sizeof(mylist));
			memset(li, 0, sizeof(mylist));
			mylist->hd = (void*)_mb;
			list_add_head(&head, &li->list);
#endif


#if 0
			// 将yuv数据写入文件
			{
				dbg("===========> yk decode h264 _mb: ptr=%p, size:%lu\n", 
						RK_MPI_MB_GetPtr(_mb), RK_MPI_MB_GetSize(_mb));
				// 写入解码后数据
				if(dec->fp_yuv)fwrite(RK_MPI_MB_GetPtr(_mb), RK_MPI_MB_GetSize(_mb), 1, dec->fp_yuv);
			}
#endif


	
#ifdef VDEC_DISPLAY
			dbg("-----------------------------------> send vo!\n");
			// 送显
			ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_VO, 0, _mb);
			if(ret != 0)
			{
				printf("error to send _mb to vo display!, will exit\n");
				exit(0);
			}

#endif
			static long long pts_old = 0, pts_new = 0;
			pts_new = RK_MPI_MB_GetTimestamp(_mb);
			dbg(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> get dec pts:%llu\n", pts_new);
			// 检测解码后pts 值是否正确
#if 0
			if(pts_old != 0)
			{
				if(pts_new - pts_old != _DIFF)
				{
					dbg("error in pts diff!!, will exit!\n");
					exit(1);
				}
			}
#endif
			pts_old = pts_new;


			// 输出宽高
#if 1
			MB_IMAGE_INFO_S stImageInfo = {0};
			ret = RK_MPI_MB_GetImageInfo(_mb, &stImageInfo);
			if (ret) 
			{
				printf("Get image info failed! ret = %d\n", ret);
				RK_MPI_MB_ReleaseBuffer(_mb);
				exit(0);
			}

			printf("Get Frame:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
					"timestamp:%lld, ImgInfo:<wxh %dx%d, fmt 0x%x>\n",
					RK_MPI_MB_GetPtr(_mb), RK_MPI_MB_GetFD(_mb), RK_MPI_MB_GetSize(_mb),
					RK_MPI_MB_GetModeID(_mb), RK_MPI_MB_GetChannelID(_mb),
					RK_MPI_MB_GetTimestamp(_mb), stImageInfo.u32Width,
					stImageInfo.u32Height, stImageInfo.enImgType);
#endif

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

			// 释放空间
			ret = RK_MPI_MB_ReleaseBuffer(_mb);
			//ret = RK_MPI_MB_release(_mb);
			if(ret != 0)
			{
				printf("error in release dec buff!\n");
				exit(1);
			}

			// 进行延时, 此处可以不延时
			//usleep(1*1000);
			//usleep(17*1000);
		}


	}

	dbg("decode frame num:%d\n", _count);
#if 0
	if(_count > 1200)
	{
		mark = 1;
	}
#endif



	// 释放数据
	RK_MPI_MB_ReleaseBuffer(mb);
#if 0
	RK_MPI_MB_ReleaseBuffer(mb_vpu);
#endif
	// 直接退出
	if(mark == 1)
	{
		return -1;
	}

	return 0;
}

static pthread_t thread_dec;
static void *test_h264_dec_proc(void *param)
{
#ifdef DEC_1080P
	char *h264 = "./tennis200.h264";
	//char *h264 = "../rk3568_1080p.h264";
	//char *h264 = "./1080P.h265";
#else
	char *h264 = "../h264_720p.h264"; // 720p 视频
#endif
	
	FILE* fp_h264_out = fopen("out.h264", "wb+");
	FILE* fp_yuv = fopen("out.yuv", "wb+");
	int ret;

	t_h264_dec dec;
	memset(&dec, 0, sizeof(t_h264_dec));

#ifdef DEC_1080P
	int w=1920;
	int h = 1080;
#else
	int w=1280;
	int h = 720;
#endif
	// 创建解码器
	ret = vpu_decode_h264_init(&dec.dec, w, h);
	if(ret != 0)
	{
		printf("error in h264 dec init,ret=%d\n", ret);
		exit(0);
	}
#if 0
	dec.fp_h264_out = fp_h264_out;
	dec.fp_yuv = fp_yuv;
	dec.dec.fp_output = fp_yuv;
#endif

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

	//list_init(&dec.head);

	// 读取h264文件
	long bytes = 0;
	uint8_t* ptr = file_read(h264, &bytes);
	while(is_start == 1) {
		uint8_t *p = ptr;
		// 解析h264文件
		ret = mpeg4_h264_annexb_nalu(p, bytes, h264_handler, (void*)&dec);
		// 文件读取结束会返回0
		//if(ret == 0){ dbg("annexb nalu return 0\n"); break;}

		// 返回退出
		if(ret < 0)
		{
			dbg("will break!\n");
			break;
		}

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

	pthread_kill(pthread_self(), SIGINT);

	return NULL;
}

void stop_test_h264_dec()
{
	dbg("will exit!\n");
	is_start = 0;
	pthread_join(thread_dec, NULL);
	dbg("will exit!\n");

#ifdef VDEC_DISPLAY
	// 释放解码显示
	deinit_dec_vo();
#endif
	dbg("will exit!\n");
}

void start_test_h264_dec()
{
	int ret;
	_dbg_init();

#if 1
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
#endif


#ifdef VDEC_DISPLAY
	// 初始化解码显示
#ifdef DEC_1080P
	init_dec_vo(1920, 1080);
#else
	init_dec_vo(1280, 720); // 720p vo
#endif

#endif

	is_start = 1;
	ret = pthread_create(&thread_dec, NULL, test_h264_dec_proc, NULL);	
	if(ret!=0 )
	{
		printf("error to create thread!\n");
		exit(0);
	}
}
