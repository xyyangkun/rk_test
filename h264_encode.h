/******************************************************************************
 *
 *       Filename:  h264_encode.h
 *
 *    Description:  h264/265 encode by mpp
 *    参考：
 *    https://github.com/sliver-chen/mpp_linux_cpp/tree/master/mpp
 *    https://github.com/MUZLATAN/MPP_ENCODE/blob/master/MppEncoder.cpp
 *
 *        Version:  1.0
 *        Created:  2022年11月07日 14时30分17秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef _H264_ENCODE_H_
#define _H264_ENCODE_H_

#include <memory.h>
#include <stdint.h>
#include <stdio.h>

#include "rockchip/mpp_common.h"
#include "rockchip/mpp_env.h"
#include "rockchip/mpp_mem.h"
#include "rockchip/rk_venc_ref.h"
#include "rockchip/mpi_enc_utils.h"
#include "rockchip/rk_mpi.h"
#include "rockchip/mpp_meta.h"
#include "rockchip/rk_venc_cmd.h"
#include "rockchip/mpp_packet.h"
#include <rockchip/rk_mpi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MpiEnc {
	// global flow control flag
    RK_U32 frm_eos;
    RK_U32 pkt_eos;
    RK_U32 frm_pkt_cnt;
    RK_S32 frame_count;
    RK_U64 stream_size;

	// src and dst
	//    FILE *fp_input;
	//    FILE *fp_output;

	// base flow context
    MppCtx ctx;
    MppApi *mpi;
    MppEncCfg cfg;
    MppEncPrepCfg prep_cfg;
    MppEncRcCfg rc_cfg;
    MppEncCodecCfg codec_cfg;
    MppEncSliceSplit split_cfg;
    MppEncOSDPltCfg osd_plt_cfg;
    MppEncOSDPlt    osd_plt;
    MppEncOSDData   osd_data;
    MppEncROIRegion roi_region[3];
    MppEncROICfg    roi_cfg;

    // input / output
    MppBufferGroup buf_grp;
    MppBuffer frm_buf;
    MppBuffer pkt_buf;
    MppEncSeiMode sei_mode;
    MppEncHeaderMode header_mode;

    // paramter for resource malloc
    RK_U32 width;
    RK_U32 height;
    RK_U32 hor_stride;
    RK_U32 ver_stride;
    MppFrameFormat fmt;
    MppCodingType type;
    RK_S32 num_frames;
    RK_S32 loop_times;
    // CamSource *cam_ctx;

    // resources
    size_t header_size;
    size_t frame_size;
    /* NOTE: packet buffer may overflow */
    size_t packet_size;

    RK_U32 osd_enable;
    RK_U32 osd_mode;
    RK_U32 split_mode;
    RK_U32 split_arg;

    RK_U32 user_data_enable;
    RK_U32 roi_enable;

    // rate control runtime parameter

    RK_S32 fps_in_flex;
    RK_S32 fps_in_den;
    RK_S32 fps_in_num;
    RK_S32 fps_out_flex;
    RK_S32 fps_out_den;
    RK_S32 fps_out_num;
    RK_S32 bps;
    RK_S32 bps_max;
    RK_S32 bps_min;
    RK_S32 rc_mode;
    RK_S32 gop_mode;
    RK_S32 gop_len;
    RK_S32 vi_len;
} MpiEncData;

typedef struct MpiEncArgs_t {
    char                *file_input;
    char                *file_output;
    char                *file_cfg;
    // dictionary          *cfg_ini;

    MppCodingType       type;
    MppFrameFormat      format;
    RK_S32              num_frames;
    RK_S32              loop_cnt;

    RK_S32              width;
    RK_S32              height;
    RK_S32              hor_stride;
    RK_S32              ver_stride;

    RK_S32              bps_target;
    RK_S32              bps_max;
    RK_S32              bps_min;
    RK_S32              rc_mode;

    RK_S32              fps_in_flex;
    RK_S32              fps_in_num;
    RK_S32              fps_in_den;
    RK_S32              fps_out_flex;
    RK_S32              fps_out_num;
    RK_S32              fps_out_den;

    RK_S32              gop_mode;
    RK_S32              gop_len;
    RK_S32              vi_len;

    MppEncHeaderMode    header_mode;

    MppEncSliceSplit    split;
} MpiEncArgs;

typedef struct MppEncCpbInfo_t {
	RK_S32 dpb_size;
	RK_S32 max_lt_cnt;
	RK_S32 max_st_cnt;
	RK_S32 max_lt_idx;
	RK_S32 max_st_tid;
	/* loop length of st/lt config */
	RK_S32 lt_gop;
	RK_S32 st_gop;
} MppEncCpbInfo;

typedef struct MppEncRefCfgImpl_t {
	const char *name;
	RK_S32 ready;
	RK_U32 debug;

	/* config from user */
	RK_S32 max_lt_cfg;
	RK_S32 max_st_cfg;
	RK_S32 lt_cfg_cnt;
	RK_S32 st_cfg_cnt;
	MppEncRefLtFrmCfg *lt_cfg;
	MppEncRefStFrmCfg *st_cfg;

	/* generated parameter for MppEncRefs */
	MppEncCpbInfo cpb_info;
} MppEncRefCfgImpl;



struct vpu_encode {
	MppCtx mpp_ctx;
	MppApi* mpi;
	int width;
	int height;
	RK_U8* enc_out_data;
	RK_U32 enc_out_length;
	MppPacket packet;

	// input / output
	MppBufferGroup buf_grp;
	MppBuffer frm_buf;
	MppBuffer pkt_buf;
	MppEncSeiMode sei_mode;
	MppEncHeaderMode header_mode;

	MpiEncArgs args_;
};


// 更改分辨率，帧率， 码率？


int h264_encode_init(struct h264_encode* encode,
        int width,
        int height,
        int quant,
        MppFrameFormat format);
int h264_encode_doing(struct h264_encode* encode,
        void* srcbuf,
        int src_fd,
        size_t src_size,
        void *dst_buf,
        int dst_fd,
        size_t dst_size);
void h264_encode_done(struct vpu_encode* encode);


#ifdef __cplusplus
}
#endif

#endif//_H264_ENCODE_H_
