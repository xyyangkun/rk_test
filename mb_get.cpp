/******************************************************************************
 *
 *       Filename:  mb_get.cpp
 *
 *    Description:  mpp buffer 转 media_buff
 *    时间戳 mpp_frame_get_pts(mppframe)
 *    mb->SetUSTimeStamp(mpp_frame_get_pts(mppframe)); 
 *    auto pts = mpp_frame_get_pts(frame);
 *    mpp_frame_get_pts
 *    mpp_frame_set_pts
 *    mpp_frame_get_dts
 *    mpp_frame_set_dts
 *
 *        Version:  1.0
 *        Created:  2022年12月06日 15时09分38秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <rockchip/rk_mpi.h>
#include "buffer.h"
#include "rkmedia_buffer.h"
#include "rkmedia_buffer_impl.h"

namespace easymedia { 

PixelFormat ConvertToPixFmt(const MppFrameFormat &mfmt);
PixelFormat ConvertToPixFmt(const MppFrameFormat &mfmt) {
  switch (mfmt) {
  case MPP_FMT_YUV420P:
    return PIX_FMT_YUV420P;
  case MPP_FMT_YUV420SP:
    return PIX_FMT_NV12;
  case MPP_FMT_YUV420SP_VU:
    return PIX_FMT_NV21;
  case MPP_FMT_YUV422P:
    return PIX_FMT_YUV422P;
  case MPP_FMT_YUV422SP:
    return PIX_FMT_NV16;
  case MPP_FMT_YUV422SP_VU:
    return PIX_FMT_NV61;
  case MPP_FMT_YUV422_YUYV:
    return PIX_FMT_YUYV422;
  case MPP_FMT_YUV422_UYVY:
    return PIX_FMT_UYVY422;
  case MPP_FMT_YUV444SP:
    return PIX_FMT_YUV444SP;
  case MPP_FMT_RGB565:
    return PIX_FMT_RGB565;
  case MPP_FMT_BGR565:
    return PIX_FMT_BGR565;
  case MPP_FMT_RGB888:
    return PIX_FMT_RGB888;
  case MPP_FMT_BGR888:
    return PIX_FMT_BGR888;
  case MPP_FMT_ARGB8888:
    return PIX_FMT_ARGB8888;
  case MPP_FMT_ABGR8888:
    return PIX_FMT_ABGR8888;
  case MPP_FMT_RGBA8888:
    return PIX_FMT_RGBA8888;
  case MPP_FMT_BGRA8888:
    return PIX_FMT_BGRA8888;
  default:
    RKMEDIA_LOGI("unsupport for mpp pixel fmt: %d\n", mfmt);
    return PIX_FMT_NONE;
  }
}

class _MPPFrameContext {
public:
#if 0
  _MPPFrameContext(std::shared_ptr<MPPContext> ctx, MppFrame f)
      : mctx(ctx), frame(f) {}
#else
  _MPPFrameContext(MppFrame f)
      : frame(f) {}
#endif
  ~_MPPFrameContext() {
	if(frame == NULL)
	{
		printf("!!!!!error in deinit!, will exit\n");
		exit(1);
	}
    if (frame)
      mpp_frame_deinit(&frame);
  }

private:
  //std::shared_ptr<MPPContext> mctx;
  MppFrame frame;
};

static int __free_mppframecontext(void *p) {
  assert(p);
  //printf("===============================> yk debug ! will release vdec buff!\n");
  delete (_MPPFrameContext *)p;
  return 0;
}

// frame may be deinit here or depends on ImageBuffer
static int SetImageBufferWithMppFrame(std::shared_ptr<ImageBuffer> ib,
                                      /*std::shared_ptr<MPPContext> mctx,*/
                                      MppFrame &frame) {
  const MppBuffer buffer = mpp_frame_get_buffer(frame);
  if (!buffer || mpp_buffer_get_size(buffer) == 0) {
    RKMEDIA_LOGI("Failed to retrieve the frame buffer\n");
    return -EFAULT;
  }
  ImageInfo &info = ib->GetImageInfo();
  info.pix_fmt = ConvertToPixFmt(mpp_frame_get_fmt(frame));
  info.width = mpp_frame_get_width(frame);
  info.height = mpp_frame_get_height(frame);
  info.vir_width = mpp_frame_get_hor_stride(frame);
  info.vir_height = mpp_frame_get_ver_stride(frame);
  size_t size = CalPixFmtSize(info);
  auto pts = mpp_frame_get_pts(frame);
  bool eos = mpp_frame_get_eos(frame) ? true : false;
  if (!ib->IsValid()) {
    //MPPFrameContext *ctx = new MPPFrameContext(mctx, frame);
    _MPPFrameContext *ctx = new _MPPFrameContext(frame);
    if (!ctx) {
      LOG_NO_MEMORY();
      return -ENOMEM;
    }
    ib->SetFD(mpp_buffer_get_fd(buffer));
    ib->SetPtr(mpp_buffer_get_ptr(buffer));
    assert(size <= mpp_buffer_get_size(buffer));
    ib->SetSize(mpp_buffer_get_size(buffer));
    ib->SetUserData(ctx, __free_mppframecontext);
  } else {
    assert(ib->GetSize() >= size);
    if (!ib->IsHwBuffer()) {
      void *ptr = ib->GetPtr();
      assert(ptr);
      RKMEDIA_LOGD("extra time-consuming memcpy to cpu!\n");
      memcpy(ptr, mpp_buffer_get_ptr(buffer), size);
      // sync to cpu?
    }
    mpp_frame_deinit(&frame);
  }
  ib->SetValidSize(size);
  ib->SetUSTimeStamp(pts);
  ib->SetEOF(eos);

  return 0;
}

#if 0
std::shared_ptr<MediaBuffer>  mpp_buff_2_mb(MppFrame mppframe)
{
	auto mb = std::make_shared<ImageBuffer>();
	if (!mb) {
		errno = ENOMEM;
		goto out;
	}
	//if (SetImageBufferWithMppFrame(mb, mpp_ctx, mppframe))
	if (SetImageBufferWithMppFrame(mb, mppframe))
		goto out;

	return mb;
}
#endif

}// namespace easymedia
using easymedia::SetImageBufferWithMppFrame;
using easymedia::ImageBuffer;

extern "C" {
//MEDIA_BUFFER RK_MPI_MB_from_mpp(MppFrame mppframe)
MEDIA_BUFFER RK_MPI_MB_from_mpp(MppFrame mppframe)
{
#if 0
  std::string strPixFormat = ImageTypeToString(pstImageInfo->enImgType);
  PixelFormat rkmediaPixFormat = StringToPixFmt(strPixFormat.c_str());
  if (rkmediaPixFormat == PIX_FMT_NONE) {
    RKMEDIA_LOGE("%s: unsupport pixformat!\n", __func__);
    return NULL;
  }
  RK_U32 buf_size = CalPixFmtSize(rkmediaPixFormat, pstImageInfo->u32HorStride,
                                  pstImageInfo->u32VerStride, 16);
  if (buf_size == 0)
    return NULL;
#endif
  MEDIA_BUFFER_IMPLE *mb = new MEDIA_BUFFER_IMPLE;
  if (!mb) {
    RKMEDIA_LOGE("%s: no space left!\n", __func__);
    printf("%s: no space left!\n", __func__);
    //return NULL;
	exit(1);
  }

#if 0
  RK_U32 u32RkmediaBufFlag = 2; // cached buffer type default
  if (u8Flag == MB_FLAG_NOCACHED)
    u32RkmediaBufFlag = 0;
  else if (u8Flag == MB_FLAG_PHY_ADDR_CONSECUTIVE)
    u32RkmediaBufFlag = 1;
#endif

#if 0
  auto &&rkmedia_mb = easymedia::MediaBuffer::Alloc(
      buf_size,
      boolHardWare ? easymedia::MediaBuffer::MemType::MEM_HARD_WARE
                   : easymedia::MediaBuffer::MemType::MEM_COMMON,
      u32RkmediaBufFlag);
  if (!rkmedia_mb) {
    delete mb;
    RKMEDIA_LOGE("%s: no space left!\n", __func__);
    return NULL;
  }

  ImageInfo rkmediaImageInfo = {rkmediaPixFormat, (int)pstImageInfo->u32Width,
                                (int)pstImageInfo->u32Height,
                                (int)pstImageInfo->u32HorStride,
                                (int)pstImageInfo->u32VerStride};
  mb->rkmedia_mb = std::make_shared<easymedia::ImageBuffer>(*(rkmedia_mb.get()),
                                                            rkmediaImageInfo);
	RK_U8 u8Flag = 0;
#else
	auto rkmedia_mb = std::make_shared<ImageBuffer>();
	if (!rkmedia_mb) {
		errno = ENOMEM;
		printf("ERROR in malloc memory\n");
		exit(1);
	}

	//if (SetImageBufferWithMppFrame(rkmedia_mb, mpp_ctx, mppframe))
	if (SetImageBufferWithMppFrame(rkmedia_mb, mppframe))
	{
		printf("ERROR in transfer mpp buffer!\n");
		exit(1);
		//goto out;
	}

	mb->rkmedia_mb = rkmedia_mb;
	//printf("yk debug %s %d!\n", __FUNCTION__, __LINE__);
#if 0
	printf("=============>>>>>>>>>>>>>>>>>>  mppframe=%p, buff size:%lu ===> %p %p\n",
			mppframe, mb->rkmedia_mb->GetValidSize(), mb, rkmedia_mb);
#endif

#endif
  //mb->rkmedia_mb->SetValidSize(buf_size);
  mb->ptr = mb->rkmedia_mb->GetPtr();
  mb->fd = mb->rkmedia_mb->GetFD();
  mb->handle = mb->rkmedia_mb->GetHandle();
  mb->dev_fd = mb->rkmedia_mb->GetDevFD();
  mb->size = mb->rkmedia_mb->GetValidSize();//buf_size;
  mb->type = MB_TYPE_IMAGE;
  //mb->stImageInfo = *pstImageInfo;
  // todo 需要时间戳
  mb->timestamp = 0;
  mb->mode_id = RK_ID_UNKNOW;
  mb->chn_id = 0;
  mb->flag = 0;
  mb->tsvc_level = 0;

  return mb;
}

int RK_MPI_MB_release(MEDIA_BUFFER mb)
{
	MEDIA_BUFFER_IMPLE *mb_impl = (MEDIA_BUFFER_IMPLE *)mb;
	if (!mb)
	{
		printf("error in rlease mb !\n");
		return -1;
	}

	// printf("============<<<<<<<<<<<<<<<<<<<<<<<<%p %p\n", (void *)mb,*(char *)&(mb_impl->rkmedia_mb));

	if (mb_impl->rkmedia_mb)
		mb_impl->rkmedia_mb.reset();


	delete mb_impl;

	return 0;
}
}// extern "C"

