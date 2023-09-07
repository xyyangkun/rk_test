/******************************************************************************
 *
 *       Filename:  rk_h264_decode.h
 *
 *    Description:  rk h264 decode 
 *    1080p解码，720p解码，转换为mb 内存管理
 *
 *        Version:  1.0
 *        Created:  2022年10月09日 17时05分14秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef RK_H264_DECODE_H_
#define RK_H264_DECODE_H_

#include <memory.h>
#include <stdint.h>
#include <stdio.h>

#include <rockchip/rk_mpi.h>

#ifdef __cplusplus
extern "C" {
#endif


#define MPP_ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))

struct vpu_h264_decode {
    int in_width;
    int in_height;
    int hor_stride;
    int ver_stride;
    RK_S32 pkt_size;
    MppCtx mpp_ctx;
    MppApi* mpi;
    //MppBufferGroup memGroup;
	MppFrameFormat fmt;

	MppBufferGroup  frm_grp;
	MppBufferGroup  pkt_grp;

	RK_U32          eos;

	RK_S32          frame_count;
	RK_S32          frame_num;
	size_t          max_usage;

	FILE *fp_output;
};


int vpu_decode_h264_init(struct vpu_h264_decode* decode, int width, int height);
//int (*handler)(void* param, void *mb))
int vpu_decode_h264_doing(struct vpu_h264_decode* decode, void *mb_in,
                          void *mb_array[], int *size);
int vpu_decode_h264_done(struct vpu_h264_decode* decode);

int vpu_decode_h264_doing_last(struct vpu_h264_decode* decode, void *mb_array[], int *size);
#ifdef __cplusplus
}
#endif

#endif//RK_H264_DECODE_H_
