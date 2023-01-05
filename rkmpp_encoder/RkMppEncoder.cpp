//
// Created by z on 2020/7/17.
//
//extern "C" {
//#include "inc/utils.h"
//#include "inc/rk_mpi.h"
//#include "inc/rk_venc_ref.h"
//#include "inc/mpp_meta.h"
//#include "inc/rk_venc_cmd.h"
//#include "inc/mpp_packet.h"
//};

#include "RkMppEncoder.h"
#include <stdio.h>
#include <fstream>

#if RKMPPENCODE_DEBUG == 1
// 进行zlog
#include "zlog_api.h"
extern zlog_category_t * log_category;
#define _log(fmt,args ...) do {\
    if(log_category)zlog_debug(log_category, fmt, ##args ); \
}while(0)


#else
#define _log(fmt,args ...)
#endif

// 不引用头文件了
// mpi_enc_utils.h
// 是现在 libutils.a
extern "C" {
MPP_RET mpi_enc_gen_ref_cfg(MppEncRefCfg ref, RK_S32 gop_mode);
MPP_RET mpi_enc_gen_smart_gop_ref_cfg(MppEncRefCfg ref, RK_S32 gop_len, RK_S32 vi_len);
RK_S32 mpi_enc_width_default_stride(RK_S32 width, MppFrameFormat fmt);
};

namespace osee {

#define MPP_ALIGN(x, a)         (((x)+(a)-1)&~((a)-1))

void MppEncoder::init() {
	MPP_RET ret = MPP_OK;

	MppPollType timeout = MPP_POLL_BLOCK;

	_log("mpi_enc_test start\n");

	ret = enc_ctx_init(&p, &args_);
	if (ret) {
		_log("test data init failed ret %d\n", ret);
		deinit();
	}
	ret = mpp_buffer_group_get_internal(&p->buf_grp, MPP_BUFFER_TYPE_DRM);
    if (ret) {
        _log("failed to get mpp buffer group ret %d\n", ret);
        deinit();
    }

    ret = mpp_buffer_get(p->buf_grp, &p->frm_buf, p->frame_size + p->header_size);
	if (ret) {
		_log("failed to get buffer for input frame ret %d\n", ret);
		deinit();
	}

    ret = mpp_buffer_get(p->buf_grp, &p->pkt_buf, p->frame_size);
	if (ret) {
		_log("failed to get buffer for input osd index ret %d\n", ret);
		deinit();
	}
	

	_log("mpi_enc_test encoder test start w %d h %d type %d\n", p->width,
				 p->height, p->type);

	// encoder demo
	ret = mpp_create(&p->ctx, &p->mpi);
	if (ret) {
		_log("mpp_create failed ret %d\n", ret);
		deinit();
	}

	ret = p->mpi->control(p->ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
	if (MPP_OK != ret) {
		_log("mpi control set output timeout %d ret %d\n", timeout, ret);
		deinit();
	}

	ret = mpp_init(p->ctx, MPP_CTX_ENC, p->type);
	if (ret) {
		_log("mpp_init failed ret %d\n", ret);
		deinit();
	}

	ret = mpp_enc_cfg_init(&p->cfg);
	if (ret) {
		_log("mpp_enc_cfg_init failed ret %d\n", ret);
		deinit();
	}


	ret = test_mpp_enc_cfg_setup(p);
	if (ret) {
		_log("test mpp setup failed ret %d\n", ret);
		deinit();
	}
}
void MppEncoder::deinit() {

    p->mpi->reset(p->ctx);

	if (p->ctx) {
		mpp_destroy(p->ctx);
		p->ctx = NULL;
	}

	if (p->cfg) {
		mpp_enc_cfg_deinit(p->cfg);
		p->cfg = NULL;
	}

	if (p->frm_buf) {
		mpp_buffer_put(p->frm_buf);
		p->frm_buf = NULL;
	}

	if (p->pkt_buf) {
		mpp_buffer_put(p->pkt_buf);
		p->pkt_buf = NULL;
	}

	// if (p->osd_idx_buf) {
	// 	mpp_buffer_put(p->osd_idx_buf);
	// 	p->osd_idx_buf = NULL;
	// }

	enc_ctx_deinit(&p);
}

MPP_RET MppEncoder::enc_ctx_deinit(MpiEncData **data) {
	MpiEncData *p = NULL;

	if (!data) {
		_log("invalid input data %p\n", data);
		return MPP_ERR_NULL_PTR;
	}

	p = *data;
	if (p) {
		//        if (p->fp_input) {
		//            fclose(p->fp_input);
		//            p->fp_input = NULL;
		//        }
		//        if (p->fp_output) {
		//            fclose(p->fp_output);
		//            p->fp_output = NULL;
		//        }
		//MPP_FREE(p);
		free(p);
		*data = NULL;
	}

	return MPP_OK;
}

MPP_RET MppEncoder::enc_ctx_init(MpiEncData **data, MpiEncArgs *cmd) {
	MpiEncData *p = NULL;
	MPP_RET ret = MPP_OK;

	if (!data || !cmd) {
		_log("invalid input data %p cmd %p\n", data, cmd);
		return MPP_ERR_NULL_PTR;
	}

	//p = mpp_calloc(MpiEncData, 1);
	p = (MpiEncData *)calloc(sizeof(MpiEncData), 1);
	if (!p) {
		_log("create MpiEncTestData failed\n");
		ret = MPP_ERR_MALLOC;
		*data = p;
		return ret;
	}

	// get paramter from cmd
    p->width        = cmd->width;
    p->height       = cmd->height;
    p->hor_stride   = (cmd->hor_stride) ? (cmd->hor_stride) :
                      (MPP_ALIGN(cmd->width, 16));
    p->ver_stride   = (cmd->ver_stride) ? (cmd->ver_stride) :
                      (MPP_ALIGN(cmd->height, 16));
    p->fmt          = cmd->format;
    p->type         = cmd->type;
    p->bps          = cmd->bps_target;
    p->bps_min      = cmd->bps_min;
    p->bps_max      = cmd->bps_max;
    p->rc_mode      = cmd->rc_mode;
    p->num_frames   = cmd->num_frames;
    if (cmd->type == MPP_VIDEO_CodingMJPEG && p->num_frames == 0) {
        _log("jpege default encode only one frame. Use -n [num] for rc case\n");
        p->num_frames   = 1;
    }
    p->gop_mode     = cmd->gop_mode;
    p->gop_len      = cmd->gop_len;
    p->vi_len       = cmd->vi_len;

    p->fps_in_flex  = cmd->fps_in_flex;
    p->fps_in_den   = cmd->fps_in_den;
    p->fps_in_num   = cmd->fps_in_num;
    p->fps_out_flex = cmd->fps_out_flex;
    p->fps_out_den  = cmd->fps_out_den;
    p->fps_out_num  = cmd->fps_out_num;

	// update resource parameter
    switch (p->fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP:
    case MPP_FMT_YUV420P: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 3 / 2;
    } break;

    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_YVYU :
    case MPP_FMT_YUV422_UYVY :
    case MPP_FMT_YUV422_VYUY :
    case MPP_FMT_YUV422P :
    case MPP_FMT_YUV422SP :
    case MPP_FMT_RGB444 :
    case MPP_FMT_BGR444 :
    case MPP_FMT_RGB555 :
    case MPP_FMT_BGR555 :
    case MPP_FMT_RGB565 :
    case MPP_FMT_BGR565 : {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 2;
    } break;

    default: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 4;
    } break;
    }

    if (MPP_FRAME_FMT_IS_FBC(p->fmt)){
        p->header_size = MPP_ALIGN(MPP_ALIGN(p->width, 16) * MPP_ALIGN(p->height, 16) / 16, 4096);
	} else {
        p->header_size = 0;
	}

	// /*
	//  * osd idx size range from 16x16 bytes(pixels) to
	//  * hor_stride*ver_stride(bytes). for general use, 1/8 Y buffer is enough.
	//  */
	// p->osd_idx_size = p->hor_stride * p->ver_stride / 8;
	// p->plt_table[0] = MPP_ENC_OSD_PLT_RED;
	// p->plt_table[1] = MPP_ENC_OSD_PLT_YELLOW;
	// p->plt_table[2] = MPP_ENC_OSD_PLT_BLUE;
	// p->plt_table[3] = MPP_ENC_OSD_PLT_GREEN;
	// p->plt_table[4] = MPP_ENC_OSD_PLT_CYAN;
	// p->plt_table[5] = MPP_ENC_OSD_PLT_TRANS;
	// p->plt_table[6] = MPP_ENC_OSD_PLT_BLACK;
	// p->plt_table[7] = MPP_ENC_OSD_PLT_WHITE;

	*data = p;
	return ret;
}

MPP_RET MppEncoder::mpi_enc_gen_osd_data(MppEncOSDData *osd_data, 
		MppBuffer osd_buf, RK_U32 frame_cnt) {
	RK_U32 k = 0;
	RK_U32 buf_size = 0;
	RK_U32 buf_offset = 0;
	RK_U8 *buf = (RK_U8 *)mpp_buffer_get_ptr(osd_buf);

	osd_data->num_region = 8;
	osd_data->buf = osd_buf;

	for (k = 0; k < osd_data->num_region; k++) {
		MppEncOSDRegion *region = &osd_data->region[k];
		RK_U8 idx = k;

		region->enable = 1;
		region->inverse = frame_cnt & 1;
		region->start_mb_x = k * 3;
		region->start_mb_y = k * 2;
		region->num_mb_x = 2;
		region->num_mb_y = 2;

		buf_size = region->num_mb_x * region->num_mb_y * 256;
		buf_offset = k * buf_size;
		osd_data->region[k].buf_offset = buf_offset;

		memset(buf + buf_offset, idx, buf_size);
	}

	return MPP_OK;
}

#if 0
MPP_RET MppEncoder::WriteHeadInfo(char *dst, int *length) {
	MPP_RET ret = MPP_OK;
	MppApi *mpi;
	MppCtx ctx;

	if (NULL == p) return MPP_ERR_NULL_PTR;

	mpi = p->mpi;
	ctx = p->ctx;

	//
	if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
		packet = NULL;
		ret = mpi->control(ctx, MPP_ENC_GET_EXTRA_INFO, &packet);
		if (ret) {
			_log("mpi control enc get extra info failed\n");
			return ret;
		}
		std::cout << "in write head 1" << std::endl;

		/* get and write sps/pps for H.264 */
		if (packet) {
			void *ptr = mpp_packet_get_pos(packet);
			*length = mpp_packet_get_length(packet);

			memcpy(dst, ptr, *length);

			packet = NULL;
			std::cout << "in write head 2" << std::endl;
		}
	}
	return ret;
}

MPP_RET MppEncoder::WriteHeadInfo(FILE* fp)
{
    MPP_RET ret = MPP_OK;
    MppApi *mpi;
    MppCtx ctx;

    if (NULL == p)
        return MPP_ERR_NULL_PTR;

    mpi = p->mpi;
    ctx = p->ctx;

    //
    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        MppPacket m_packet = NULL;

        /*
         * Can use packet with normal malloc buffer as input not pkt_buf.
         * Please refer to vpu_api_legacy.cpp for normal buffer case.
         * Using pkt_buf buffer here is just for simplifing demo.
         */
        mpp_packet_init_with_buffer(&m_packet, p->pkt_buf);
        /* NOTE: It is important to clear output packet length!! */
        mpp_packet_set_length(m_packet, 0);

        ret = mpi->control(ctx, MPP_ENC_GET_EXTRA_INFO, &m_packet);
        if (ret) {
            _log("mpi control enc get extra info failed\n");
            return ret;
        }else{
        /* get and write sps/pps for H.264 */
            void *ptr   = mpp_packet_get_pos(m_packet);
            int len  = mpp_packet_get_length(m_packet);

            fwrite(ptr, len, 1, fp);

            m_packet = NULL;
        }
		mpp_packet_deinit(&m_packet);
    }
    return ret;
}
#endif

//override
// MPP_RET MppEncoder::encode(const cv::Mat& img, char* dst, int *length)
// {
//     MPP_RET ret = MPP_OK;
//     MppApi *mpi;
//     MppCtx ctx;

// 	char *tmpP = dst;
// 	*length = 0;
// 	// for test group
// 	// mpp默认设置为IPPPPP帧的模式， p->gop 为60帧
// 	//即每隔60帧写一次头信息
// 	// std::cout << " p->gop is:" << p->gop << std::endl;
// 	if (0 == (countIdx_ % 60)) {
// 		countIdx_ = 0;

// 		int len = 0;

// 		WriteHeadInfo(tmpP, &len);
// 		tmpP += len;
// 		*length += len;
// 	}

// 	if (NULL == p) return MPP_ERR_NULL_PTR;

// 	mpi = p->mpi;
// 	ctx = p->ctx;

// 	MppFrame frame = NULL;
// 	packet = NULL;
// 	MppMeta meta = NULL;
// 	RK_U32 eoi = 1;

// 	void *buf = mpp_buffer_get_ptr(p->frm_buf);

// 	//将数据转为yuv I420 420p
// 	cv::Mat yuvImg;
// 	cv::cvtColor(img, yuvImg, CV_BGR2YUV_I420);

// 	std::cout <<"cols and rows are: "<< yuvImg.cols << "  " << yuvImg.rows << std::endl;
// 	memcpy(buf, yuvImg.data, yuvImg.cols * yuvImg.rows);

//     ret = mpp_frame_init(&frame);
//     if (ret) {
//         _log("mpp_frame_init failed\n");
//         return ret;
//     }
//     mpp_frame_set_width(frame, p->width);
//     mpp_frame_set_height(frame, p->height);
//     mpp_frame_set_hor_stride(frame, p->hor_stride);
//     mpp_frame_set_ver_stride(frame, p->ver_stride);
//     mpp_frame_set_fmt(frame, p->fmt);
//     mpp_frame_set_eos(frame, p->frm_eos);

//     mpp_frame_set_buffer(frame, p->frm_buf);
//     meta = mpp_frame_get_meta(frame);
//     mpp_packet_init_with_buffer(&packet, p->pkt_buf);
//     /* NOTE: It is important to clear output packet length!! */
//     mpp_packet_set_length(packet, 0);
//     mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, packet);
  
// 	/*
//      * NOTE: in non-block mode the frame can be resent.
//      * The default input timeout mode is block.
//      *
//      * User should release the input frame to meet the requirements of
//      * resource creator must be the resource destroyer.
//      */
//     ret = mpi->encode_put_frame(ctx, frame);
//     if (ret) {
//         _log("mpp encode put frame failed\n");
//         mpp_frame_deinit(&frame);
//         return ret;
//     }
//     mpp_frame_deinit(&frame);
//     do {
//         ret = mpi->encode_get_packet(ctx, &packet);
//         if (ret) {
//             _log("mpp encode get packet failed\n");
//             return ret;
//         }
        
// 		//    mpp_assert(packet);

//         if (packet) {
//             // write packet to file here
//             void *ptr   = mpp_packet_get_pos(packet);
//             size_t len  = mpp_packet_get_length(packet);
  
//             p->pkt_eos = mpp_packet_get_eos(packet);
//             memcpy(tmpP, ptr, len);
// 			*length += len;
            
//             /* for low delay partition encoding */
//             if (mpp_packet_is_partition(packet)) {
//                 eoi = mpp_packet_is_eoi(packet);
//                 _log(" pkt %d", p->frm_pkt_cnt);
//                 p->frm_pkt_cnt = (eoi) ? (0) : (p->frm_pkt_cnt + 1);
//             }
            
//             if (mpp_packet_has_meta(packet)) {
//                 meta = mpp_packet_get_meta(packet);
//                 RK_S32 temporal_id = 0;
//                 RK_S32 lt_idx = -1;
//                 RK_S32 avg_qp = -1;
//                 if (MPP_OK == mpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &temporal_id))
//                 	_log(" tid %d", temporal_id);
//                 if (MPP_OK == mpp_meta_get_s32(meta, KEY_LONG_REF_IDX, &lt_idx))
//                     _log(" lt %d", lt_idx);
//                 if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_AVERAGE_QP, &avg_qp))
//                     _log(" qp %d", avg_qp);
//             }
//             mpp_packet_deinit(&packet);
//             p->stream_size += len;
//             p->frame_count += eoi;
//             if (p->pkt_eos) {
//                 _log("%p found last packet\n", ctx);

//             }
//         }
//     } while (!eoi);

    

// 	return ret;
// }
//int MppEncoder::set_encparam(int bitrate)

// 设置码率
int MppEncoder::set_bitrate(int bitrate)
{
	int ret = 0;
	MppEncCfg enc_cfg;
	RK_S32              bps_target;
    RK_S32              bps_max;
    RK_S32              bps_min;
    RK_S32              rc_mode;

	rc_mode = MPP_ENC_RC_MODE_CBR;
	bps_min = bitrate;
	bps_max = bitrate;
	bps_target = bitrate;

	enc_cfg = p->cfg;


	ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:mode", rc_mode);
	ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_min", bps_min);
	ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_max", bps_max);
	ret |= mpp_enc_cfg_set_s32(enc_cfg, "rc:bps_target", bps_target);
	if (ret) 
	{ 
		_log("ERROR in mpp_enc_cfg_set_s32!\n");
		return -1;
	}
	// MPP_ENC_SET_CFG
	MpiCmd mpi_cmd = MPP_ENC_SET_CFG;
	ret = p->mpi->control(p->ctx, mpi_cmd, (MppParam)enc_cfg);
	if (ret)
	{
		_log("ERROR in mpp control!\n");
		return -2;
	}

	return 0;
}

// 设置帧率
int MppEncoder::set_fps(int fps)
{
	int ret = 0;
	MppApi *mpi;
	MppCtx ctx;
	MppEncCfg cfg;	

	mpi = p->mpi;
	ctx = p->ctx;
	cfg = p->cfg;

	p->fps_in_den = 1;
	p->fps_in_num = fps;
	p->fps_out_den = 1;
	p->fps_out_num = fps;
	p->gop_len = fps;

	ret |= mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", p->fps_in_num);
	ret |= mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denorm", p->fps_in_den);
	ret |= mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", p->fps_out_num);
	ret |= mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denorm", p->fps_out_den);
    ret |= mpp_enc_cfg_set_s32(cfg, "rc:gop", p->gop_len); // gop
	if (ret) 
	{ 
		_log("ERROR in mpp_enc_cfg_set_s32!\n");
		return -1;
	}

	// MPP_ENC_SET_CFG
	MpiCmd mpi_cmd = MPP_ENC_SET_CFG;
	ret = mpi->control(ctx, mpi_cmd, (MppParam)cfg);
	if (ret)
	{
		_log("ERROR in mpp control!\n");
		return -2;
	}

	return 0;
}

// 设置I帧间隔
int MppEncoder::set_gop(int gop)
{
	int ret = 0;
	MppApi *mpi;
	MppCtx ctx;
	MppEncCfg cfg;	

	mpi = p->mpi;
	ctx = p->ctx;
	cfg = p->cfg;

	 ret |= mpp_enc_cfg_set_s32(cfg, "rc:gop", gop);
	 MpiCmd mpi_cmd = MPP_ENC_SET_CFG;
	 ret = mpi->control(ctx, mpi_cmd, cfg);
	if (ret)
	{
		_log("ERROR in mpp control!\n");
		return -2;
	}

	return 0;
}



//MPP_RET MppEncoder::encode(const void* mb_in, char* dst, int *length)
MPP_RET MppEncoder::encode(const void* mb_in, void *mb_out)
{
    MPP_RET ret = MPP_OK;
    MppApi *mpi;
    MppCtx ctx;
	MppFrame frame = NULL;

	MEDIA_BUFFER mb = (MEDIA_BUFFER)mb_in;
	void* in_data;
	RK_S32 in_size;
	in_data = RK_MPI_MB_GetPtr(mb);
	in_size = RK_MPI_MB_GetSize(mb);
	RK_U64 pts = RK_MPI_MB_GetTimestamp(mb);
	RK_U64 dts = RK_MPI_MB_GetTimestamp(mb);
	int fd = RK_MPI_MB_GetFD(mb);;

	// 转化成mpp_info
	MppBuffer buffer = nullptr;
	MppBufferInfo info;
	memset(&info, 0, sizeof(info));
	info.type = MPP_BUFFER_TYPE_ION;
	info.fd = fd;
	info.ptr = in_data;
	info.size = in_size;
	// MppBuffer &buffer, MEDIA_BUFFER 转换为MppBuffer
	ret = mpp_buffer_import(&buffer, &info);
	if (ret) {
		_log("import input picture buffer failed\n");
		exit(1);
	}

	////////////////////////////////mb out/////////////////////////////////////
	packet = NULL;

	MEDIA_BUFFER omb = (MEDIA_BUFFER)mb_out;
#if 0
	void* out_data;
	RK_S32 out_size;
	out_data = RK_MPI_MB_GetPtr(omb);
	out_size = RK_MPI_MB_GetSize(omb);
	int out_fd = RK_MPI_MB_GetFD(omb);;

	// 转化成mpp_info
	MppBuffer out_buffer = nullptr;
	MppBufferInfo out_info;
	memset(&out_info, 0, sizeof(out_info));
	out_info.type = MPP_BUFFER_TYPE_ION;
	out_info.fd = out_fd;
	out_info.ptr = out_data;
	out_info.size = out_size;
	_log("out_fd=%d ptr:%p size:%d\n", out_fd, out_data, out_size);
	// MppBuffer &out_buffer, MEDIA_BUFFER 转换为MppBuffer
	ret = mpp_buffer_import(&out_buffer, &out_info);
	if (ret != MPP_OK) {
		_log("import input picture out_buffer failed\n");
		exit(1);
	}

    ret = mpp_packet_init_with_buffer(&packet, out_buffer);
	if (ret != MPP_OK) {
		_log("init out_buffer failed\n");
		exit(1);
	}
	mpp_buffer_put(out_buffer);
#endif
	////////////////////////////////mb out/////////////////////////////////////




	//char *tmpP = dst;
	//*length = 0;

	_log("!!!%s %d\n", __FUNCTION__, __LINE__);

	mpi = p->mpi;
	ctx = p->ctx;

	MppMeta meta = NULL;
	RK_U32 eoi = 1;

    ret = mpp_frame_init(&frame);
    if (ret) {
        _log("mpp_frame_init failed\n");
        return ret;
    }
    mpp_frame_set_width(frame, p->width);
    mpp_frame_set_height(frame, p->height);
    mpp_frame_set_hor_stride(frame, p->hor_stride);
    mpp_frame_set_ver_stride(frame, p->ver_stride);
    mpp_frame_set_fmt(frame, p->fmt);
    mpp_frame_set_eos(frame, p->frm_eos);

	// 设置dts pts
	mpp_frame_set_dts(frame, dts);
	mpp_frame_set_pts(frame, pts);

#if 0
    mpp_frame_set_buffer(frame, p->frm_buf);
#else
	mpp_frame_set_buffer(frame, buffer);
	// 如果是结尾，要设置
	//mpp_frame_set_eos(frame, 1);
	
	mpp_buffer_put(buffer);
#endif

	_log("!!!%s %d\n", __FUNCTION__, __LINE__);

    //meta = mpp_frame_get_meta(frame);
    //mpp_packet_init_with_buffer(&packet, p->pkt_buf);
	
	

    /* NOTE: It is important to clear output packet length!! */
    //mpp_packet_set_length(packet, 0);
    //mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, packet);
  
	_log("!!!%s %d\n", __FUNCTION__, __LINE__);
	/*
     * NOTE: in non-block mode the frame can be resent.
     * The default input timeout mode is block.
     *
     * User should release the input frame to meet the requirements of
     * resource creator must be the resource destroyer.
     */
    ret = mpi->encode_put_frame(ctx, frame);
    if (ret) {
        _log("mpp encode put frame failed\n");
        mpp_frame_deinit(&frame);
			exit(1);
        return ret;
    }
	_log("!!!%s %d\n", __FUNCTION__, __LINE__);
    mpp_frame_deinit(&frame);
	_log("!!!%s %d eoi=%d\n", __FUNCTION__, __LINE__, eoi);
    do {
        ret = mpi->encode_get_packet(ctx, &packet);
        if (ret) {
            _log("mpp encode get packet failed\n");
			exit(1);
            return ret;
        }
        
		//    mpp_assert(packet);

        if (packet) {
            // write packet to file here
            void *ptr   = mpp_packet_get_pos(packet);
            size_t len  = mpp_packet_get_length(packet);
			// RK_S64 pts_out = mpp_packet_get_pts(frame);
  
            p->pkt_eos = mpp_packet_get_eos(packet);
            
            /* for low delay partition encoding */
            if (mpp_packet_is_partition(packet)) {
                eoi = mpp_packet_is_eoi(packet);
                _log(" pkt %d eoi=%d", p->frm_pkt_cnt, eoi);
                p->frm_pkt_cnt = (eoi) ? (0) : (p->frm_pkt_cnt + 1);
				exit(1);
            }
            
            if (mpp_packet_has_meta(packet)) {
                meta = mpp_packet_get_meta(packet);
                RK_S32 temporal_id = 0;
                RK_S32 lt_idx = -1;
                RK_S32 avg_qp = -1;
                if (MPP_OK == mpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &temporal_id))
                	_log(" tid %d", temporal_id);
                if (MPP_OK == mpp_meta_get_s32(meta, KEY_LONG_REF_IDX, &lt_idx))
                    _log(" lt %d", lt_idx);
                if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_AVERAGE_QP, &avg_qp))
                    _log(" qp %d", avg_qp);
            }
            p->stream_size += len;
            p->frame_count += eoi;
            if (p->pkt_eos) {
                _log("%p found last packet\n", ctx);

            }

			// 设置mb 属性
			//output->SetValidSize(packet_len);
			//output->SetUSTimeStamp(pts);
			//output->SetEOF(out_eof);
			// 编码后的数据仅仅需要数据长度，是否是I真可以从数据中判断
#if 1
			_log("\n\n==========================> packet size:%ld, ptr:%p %p\n",
					len, ptr, RK_MPI_MB_GetPtr(omb));
			memcpy(RK_MPI_MB_GetPtr(omb),  ptr, len);
			RK_MPI_MB_SetSize(omb, len);
			//RK_MPI_MB_SetTimestamp(omb, pts_out);
#endif

			// 最后释放packet
            mpp_packet_deinit(&packet);

        }
    } while (!eoi);
	_log("!!!%s %d\n", __FUNCTION__, __LINE__);

    

	return ret;
}

#if 0
MPP_RET MppEncoder::encode(const void* img, int img_len, char* dst, int *length)
{
    MPP_RET ret = MPP_OK;
    MppApi *mpi;
    MppCtx ctx;

	char *tmpP = dst;
	*length = 0;
	// for test group
	// mpp默认设置为IPPPPP帧的模式， p->gop 为60帧
	//即每隔60帧写一次头信息
	//std::cout << " p->gop is:" << p->gop << std::endl;
	_log("!!!%s %d\n", __FUNCTION__, __LINE__);
#if 0
	if (0 == (countIdx_ % 60)) {
		countIdx_ = 0;

		int len = 0;

		WriteHeadInfo(tmpP, &len);
		tmpP += len;
		*length += len;
	}

#endif
	_log("!!!%s %d\n", __FUNCTION__, __LINE__);
	if (NULL == p) return MPP_ERR_NULL_PTR;
	_log("!!!%s %d\n", __FUNCTION__, __LINE__);

	mpi = p->mpi;
	ctx = p->ctx;

	MppFrame frame = NULL;
	packet = NULL;
	MppMeta meta = NULL;
	RK_U32 eoi = 1;

	void *buf = mpp_buffer_get_ptr(p->frm_buf);
	memcpy(buf, img, img_len);
    ret = mpp_frame_init(&frame);
    if (ret) {
        _log("mpp_frame_init failed\n");
        return ret;
    }
    mpp_frame_set_width(frame, p->width);
    mpp_frame_set_height(frame, p->height);
    mpp_frame_set_hor_stride(frame, p->hor_stride);
    mpp_frame_set_ver_stride(frame, p->ver_stride);
    mpp_frame_set_fmt(frame, p->fmt);
    mpp_frame_set_eos(frame, p->frm_eos);

    mpp_frame_set_buffer(frame, p->frm_buf);
    meta = mpp_frame_get_meta(frame);
    mpp_packet_init_with_buffer(&packet, p->pkt_buf);
    /* NOTE: It is important to clear output packet length!! */
    mpp_packet_set_length(packet, 0);
    mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, packet);
  
	/*
     * NOTE: in non-block mode the frame can be resent.
     * The default input timeout mode is block.
     *
     * User should release the input frame to meet the requirements of
     * resource creator must be the resource destroyer.
     */
    ret = mpi->encode_put_frame(ctx, frame);
    if (ret) {
        _log("mpp encode put frame failed\n");
        mpp_frame_deinit(&frame);
        return ret;
    }
    mpp_frame_deinit(&frame);
	_log("!!!%s %d\n", __FUNCTION__, __LINE__);
    do {
        ret = mpi->encode_get_packet(ctx, &packet);
        if (ret) {
            _log("mpp encode get packet failed\n");
            return ret;
        }
        
		//    mpp_assert(packet);

        if (packet) {
            // write packet to file here
            void *ptr   = mpp_packet_get_pos(packet);
            size_t len  = mpp_packet_get_length(packet);
  
            p->pkt_eos = mpp_packet_get_eos(packet);
            memcpy(tmpP, ptr, len);
			*length += len;
            
            /* for low delay partition encoding */
            if (mpp_packet_is_partition(packet)) {
                eoi = mpp_packet_is_eoi(packet);
                _log(" pkt %d", p->frm_pkt_cnt);
                p->frm_pkt_cnt = (eoi) ? (0) : (p->frm_pkt_cnt + 1);
            }
            
            if (mpp_packet_has_meta(packet)) {
                meta = mpp_packet_get_meta(packet);
                RK_S32 temporal_id = 0;
                RK_S32 lt_idx = -1;
                RK_S32 avg_qp = -1;
                if (MPP_OK == mpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &temporal_id))
                	_log(" tid %d", temporal_id);
                if (MPP_OK == mpp_meta_get_s32(meta, KEY_LONG_REF_IDX, &lt_idx))
                    _log(" lt %d", lt_idx);
                if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_AVERAGE_QP, &avg_qp))
                    _log(" qp %d", avg_qp);
            }
            mpp_packet_deinit(&packet);
            p->stream_size += len;
            p->frame_count += eoi;
            if (p->pkt_eos) {
                _log("%p found last packet\n", ctx);

            }
        }
    } while (!eoi);
	_log("!!!%s %d\n", __FUNCTION__, __LINE__);

    

	return ret;
}
#endif



// void MppEncoder::calcSize(int src_w, int src_h) {
// 	int cond1 = ceil(src_w * 1.0 / 16) * 16 - src_w;
// 	int cond2 = ceil(src_h * 1.0 / 16) * 16 - src_h;

// 	std::cout << cond1 << "  " << cond2 << std::endl;

// 	float w_rate = 0.0, h_rate = 0.0;
// 	if (cond1 > 8 && cond2 > 8) {
// 		append = false;
// 		w_rate = floor((src_w * 1.0) / 16);
// 		h_rate = floor((src_h * 1.0) / 16);
// 		left = (src_w - w_rate * 16) / 2;
// 		top = (src_h - h_rate * 16) / 2;

// 		rect_.x = left;
// 		rect_.y = top;
// 		rect_.width = w_rate * 16;
// 		rect_.height = h_rate * 16;

// 		std::cout << rect_ << std::endl;

// 	} else {
// 		append = true;

// 		w_rate = ceil(src_w * 1.0 / 16);
// 		h_rate = ceil(src_h * 1.0 / 16);

// 		left = (w_rate * 16 - src_w) / 2;
// 		right = (w_rate * 16 - src_w) - left;
// 		top = (h_rate * 16 - src_h) / 2;
// 		bottom = (h_rate * 16 - src_h) - top;
// 	}

// 	args_.width = w_rate * 16;
// 	args_.height = h_rate * 16;
// }

// MPP_RET MppEncoder::mpi_enc_gen_ref_cfg(MppEncRefCfg ref) {
// 	MppEncRefLtFrmCfg lt_ref[4];
// 	MppEncRefStFrmCfg st_ref[16];
// 	RK_S32 lt_cnt = 1;
// 	RK_S32 st_cnt = 9;
// 	MPP_RET ret = MPP_OK;

// 	memset(&lt_ref, 0, sizeof(lt_ref));
// 	memset(&st_ref, 0, sizeof(st_ref));

// 	ret = mpp_enc_ref_cfg_set_cfg_cnt(ref, lt_cnt, st_cnt);

// 	/* set 8 frame lt-ref gap */
// 	lt_ref[0].lt_idx = 0;
// 	lt_ref[0].temporal_id = 0;
// 	lt_ref[0].ref_mode = REF_TO_PREV_LT_REF;
// 	lt_ref[0].lt_gap = 8;
// 	lt_ref[0].lt_delay = 0;

// 	ret = mpp_enc_ref_cfg_add_lt_cfg(ref, 1, lt_ref);

// 	/* set tsvc4 st-ref struct */
// 	/* st 0 layer 0 - ref */
// 	st_ref[0].is_non_ref = 0;
// 	st_ref[0].temporal_id = 0;
// 	st_ref[0].ref_mode = REF_TO_TEMPORAL_LAYER;
// 	st_ref[0].ref_arg = 0;
// 	st_ref[0].repeat = 0;
// 	/* st 1 layer 3 - non-ref */
// 	st_ref[1].is_non_ref = 1;
// 	st_ref[1].temporal_id = 3;
// 	st_ref[1].ref_mode = REF_TO_PREV_REF_FRM;
// 	st_ref[1].ref_arg = 0;
// 	st_ref[1].repeat = 0;
// 	/* st 2 layer 2 - ref */
// 	st_ref[2].is_non_ref = 0;
// 	st_ref[2].temporal_id = 2;
// 	st_ref[2].ref_mode = REF_TO_PREV_REF_FRM;
// 	st_ref[2].ref_arg = 0;
// 	st_ref[2].repeat = 0;
// 	/* st 3 layer 3 - non-ref */
// 	st_ref[3].is_non_ref = 1;
// 	st_ref[3].temporal_id = 3;
// 	st_ref[3].ref_mode = REF_TO_PREV_REF_FRM;
// 	st_ref[3].ref_arg = 0;
// 	st_ref[3].repeat = 0;
// 	/* st 4 layer 1 - ref */
// 	st_ref[4].is_non_ref = 0;
// 	st_ref[4].temporal_id = 1;
// 	st_ref[4].ref_mode = REF_TO_PREV_LT_REF;
// 	st_ref[4].ref_arg = 0;
// 	st_ref[4].repeat = 0;
// 	/* st 5 layer 3 - non-ref */
// 	st_ref[5].is_non_ref = 1;
// 	st_ref[5].temporal_id = 3;
// 	st_ref[5].ref_mode = REF_TO_PREV_REF_FRM;
// 	st_ref[5].ref_arg = 0;
// 	st_ref[5].repeat = 0;
// 	/* st 6 layer 2 - ref */
// 	st_ref[6].is_non_ref = 0;
// 	st_ref[6].temporal_id = 2;
// 	st_ref[6].ref_mode = REF_TO_PREV_REF_FRM;
// 	st_ref[6].ref_arg = 0;
// 	st_ref[6].repeat = 0;
// 	/* st 7 layer 3 - non-ref */
// 	st_ref[7].is_non_ref = 1;
// 	st_ref[7].temporal_id = 3;
// 	st_ref[7].ref_mode = REF_TO_PREV_REF_FRM;
// 	st_ref[7].ref_arg = 0;
// 	st_ref[7].repeat = 0;
// 	/* st 8 layer 0 - ref */
// 	st_ref[8].is_non_ref = 0;
// 	st_ref[8].temporal_id = 0;
// 	st_ref[8].ref_mode = REF_TO_TEMPORAL_LAYER;
// 	st_ref[8].ref_arg = 0;
// 	st_ref[8].repeat = 0;

// 	ret = mpp_enc_ref_cfg_add_st_cfg(ref, 9, st_ref);

// 	/* check and get dpb size */
// 	ret = mpp_enc_ref_cfg_check(ref);

// 	return ret;
// }

MPP_RET MppEncoder::mpi_enc_gen_osd_plt(MppEncOSDPlt *osd_plt, RK_U32 *table) {
	RK_U32 k = 0;

	if (osd_plt->data && table) {
		for (k = 0; k < 256; k++) osd_plt->data[k].val = table[k % 8];
	}
	return MPP_OK;
}

MPP_RET MppEncoder::test_mpp_setup_legacy(MpiEncData *p) {
	MPP_RET ret;
	MppApi *mpi;
	MppCtx ctx;
	MppEncCodecCfg *codec_cfg;
	MppEncPrepCfg *prep_cfg;
	MppEncRcCfg *rc_cfg;
	MppEncSliceSplit *split_cfg;

	if (NULL == p) return MPP_ERR_NULL_PTR;

	mpi = p->mpi;
	ctx = p->ctx;
	codec_cfg = &p->codec_cfg;
	prep_cfg = &p->prep_cfg;
	rc_cfg = &p->rc_cfg;
	split_cfg = &p->split_cfg;

	/* setup default parameter */
	if (p->fps_in_den == 0) p->fps_in_den = 1;
	if (p->fps_in_num == 0) p->fps_in_num = 30;
	if (p->fps_out_den == 0) p->fps_out_den = 1;
	if (p->fps_out_num == 0) p->fps_out_num = 30;
	// p->gop = 60;

	if (!p->bps)
		p->bps = p->width * p->height / 8 * (p->fps_out_num / p->fps_out_den);

	prep_cfg->change = MPP_ENC_PREP_CFG_CHANGE_INPUT |
										 MPP_ENC_PREP_CFG_CHANGE_ROTATION |
										 MPP_ENC_PREP_CFG_CHANGE_FORMAT;
	prep_cfg->width = p->width;
	prep_cfg->height = p->height;
	prep_cfg->hor_stride = p->hor_stride;
	prep_cfg->ver_stride = p->ver_stride;
	prep_cfg->format = p->fmt;
	prep_cfg->rotation = MPP_ENC_ROT_0;
	ret = mpi->control(ctx, MPP_ENC_SET_PREP_CFG, prep_cfg);
	if (ret) {
		_log("mpi control enc set prep cfg failed ret %d\n", ret);
		return ret;
	}

	rc_cfg->change = MPP_ENC_RC_CFG_CHANGE_ALL;
	rc_cfg->rc_mode = MPP_ENC_RC_MODE_CBR;
	rc_cfg->quality = MPP_ENC_RC_QUALITY_MEDIUM;

	if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_FIXQP) {
		/* constant QP does not have bps */
		rc_cfg->bps_target = -1;
		rc_cfg->bps_max = -1;
		rc_cfg->bps_min = -1;
	} else if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_CBR) {
		/* constant bitrate has very small bps range of 1/16 bps */
		rc_cfg->bps_target = p->bps;
		rc_cfg->bps_max = p->bps * 17 / 16;
		rc_cfg->bps_min = p->bps * 15 / 16;
	} else if (rc_cfg->rc_mode == MPP_ENC_RC_MODE_VBR) {
		/* variable bitrate has large bps range */
		rc_cfg->bps_target = p->bps;
		rc_cfg->bps_max = p->bps * 17 / 16;
		rc_cfg->bps_min = p->bps * 1 / 16;
	}

	/* fix input / output frame rate */
	rc_cfg->fps_in_flex = p->fps_in_flex;
	rc_cfg->fps_in_num = p->fps_in_num;
	rc_cfg->fps_in_denorm = p->fps_in_den;
	rc_cfg->fps_out_flex = p->fps_out_flex;
	rc_cfg->fps_out_num = p->fps_out_num;
	rc_cfg->fps_out_denorm = p->fps_out_den;

	rc_cfg->gop = p->gop_len;
	rc_cfg->max_reenc_times = 1;

	_log("mpi_enc_test bps %d fps %d gop %d\n", rc_cfg->bps_target,
				 rc_cfg->fps_out_num, rc_cfg->gop);
	ret = mpi->control(ctx, MPP_ENC_SET_RC_CFG, rc_cfg);
	if (ret) {
		_log("mpi control enc set rc cfg failed ret %d\n", ret);
		return ret;
	}

	codec_cfg->coding = p->type;
	switch (codec_cfg->coding) {
		case MPP_VIDEO_CodingAVC: {
			codec_cfg->h264.change = MPP_ENC_H264_CFG_CHANGE_PROFILE |
															 MPP_ENC_H264_CFG_CHANGE_ENTROPY |
															 MPP_ENC_H264_CFG_CHANGE_TRANS_8x8;
			/*
			 * H.264 profile_idc parameter
			 * 66  - Baseline profile
			 * 77  - Main profile
			 * 100 - High profile
			 */
			codec_cfg->h264.profile = 100;
			/*
			 * H.264 level_idc parameter
			 * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
			 * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
			 * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
			 * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
			 * 50 / 51 / 52         - 4K@30fps
			 */
			codec_cfg->h264.level = 40;
			codec_cfg->h264.entropy_coding_mode = 1;
			codec_cfg->h264.cabac_init_idc = 0;
			codec_cfg->h264.transform8x8_mode = 1;
		} break;
		case MPP_VIDEO_CodingMJPEG: {
			codec_cfg->jpeg.change = MPP_ENC_JPEG_CFG_CHANGE_QP;
			codec_cfg->jpeg.quant = 10;
		} break;
		case MPP_VIDEO_CodingVP8: {
		} break;
		case MPP_VIDEO_CodingHEVC: {
			codec_cfg->h265.change =
					MPP_ENC_H265_CFG_INTRA_QP_CHANGE | MPP_ENC_H265_CFG_RC_QP_CHANGE;
			if (rc_cfg->rc_mode != MPP_ENC_RC_MODE_FIXQP)
				codec_cfg->h265.qp_init = -1;
			else
				codec_cfg->h265.qp_init = 26;
			codec_cfg->h265.max_i_qp = 46;
			codec_cfg->h265.min_i_qp = 24;
			codec_cfg->h265.max_qp = 51;
			codec_cfg->h265.min_qp = 10;
			if (0) {
				codec_cfg->h265.change |= MPP_ENC_H265_CFG_SLICE_CHANGE;
				codec_cfg->h265.slice_cfg.split_enable = 1;
				codec_cfg->h265.slice_cfg.split_mode = 1;
				codec_cfg->h265.slice_cfg.slice_size = 10;
			}
		} break;
		default: {
			_log("support encoder coding type %d\n", codec_cfg->coding);
		} break;
	}

	ret = mpi->control(ctx, MPP_ENC_SET_CODEC_CFG, codec_cfg);
	if (ret) {
		_log("mpi control enc set codec cfg failed ret %d\n", ret);
		return ret;
	}

	p->split_mode = 0;
	p->split_arg = 0;

	//mpp_env_get_u32("split_mode", &p->split_mode, MPP_ENC_SPLIT_NONE);
	//mpp_env_get_u32("split_arg", &p->split_arg, 0);
	p->split_mode = MPP_ENC_SPLIT_NONE;
	p->split_arg = 0;

	if (p->split_mode) {
		split_cfg->change = MPP_ENC_SPLIT_CFG_CHANGE_ALL;
		split_cfg->split_mode = p->split_mode;
		split_cfg->split_arg = p->split_arg;

		_log("split_mode %d split_arg %d\n", p->split_mode, p->split_arg);

		ret = mpi->control(ctx, MPP_ENC_SET_SPLIT, split_cfg);
		if (ret) {
			_log("mpi control enc set codec cfg failed ret %d\n", ret);
			return ret;
		}
	}

	/* optional */
	p->sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
	ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &p->sei_mode);
	if (ret) {
		_log("mpi control enc set sei cfg failed ret %d\n", ret);
		return ret;
	}

	if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
		p->header_mode = MPP_ENC_HEADER_MODE_EACH_IDR;
		ret = mpi->control(ctx, MPP_ENC_SET_HEADER_MODE, &p->header_mode);
		if (ret) {
			_log("mpi control enc set header mode failed ret %d\n", ret);
			return ret;
		}
	}

	RK_U32 gop_mode = 0;

	//mpp_env_get_u32("gop_mode", &gop_mode, 0);
	if (p->gop_mode || gop_mode) {
		MppEncRefCfg ref;

		mpp_enc_ref_cfg_init(&ref);
		// mpi_enc_gen_ref_cfg(ref);
		ret = mpi->control(ctx, MPP_ENC_SET_REF_CFG, ref);
		if (ret) {
			_log("mpi control enc set ref cfg failed ret %d\n", ret);
			return ret;
		}
		mpp_enc_ref_cfg_deinit(&ref);
	}

	return ret;
}

MPP_RET MppEncoder::test_mpp_enc_cfg_setup(MpiEncData *p) {
	MPP_RET ret;
	MppApi *mpi;
	MppCtx ctx;
	MppEncCfg cfg;	
	MppEncRcMode rc_mode = MPP_ENC_RC_MODE_AVBR;

	if (NULL == p) return MPP_ERR_NULL_PTR;

	mpi = p->mpi;
	ctx = p->ctx;
	cfg = p->cfg;

	/* setup default parameter */
	if (p->fps_in_den == 0) p->fps_in_den = 1;
	if (p->fps_in_num == 0) p->fps_in_num = 30;
	if (p->fps_out_den == 0) p->fps_out_den = 1;
	if (p->fps_out_num == 0) p->fps_out_num = 30;
	// p->gop = 60;

	if (!p->bps)
		p->bps = p->width * p->height / 8 * (p->fps_out_num / p->fps_out_den);

	mpp_enc_cfg_set_s32(cfg, "prep:width", p->width);
	mpp_enc_cfg_set_s32(cfg, "prep:height", p->height);
	mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", p->hor_stride);
	mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", p->ver_stride);
	mpp_enc_cfg_set_s32(cfg, "prep:format", p->fmt);

	mpp_enc_cfg_set_s32(cfg, "rc:mode", rc_mode);

	/* fix input / output frame rate */
	mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", p->fps_in_flex);
	mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", p->fps_in_num);
	mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denorm", p->fps_in_den);
	mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", p->fps_out_flex);
	mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", p->fps_out_num);
	mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denorm", p->fps_out_den);
    mpp_enc_cfg_set_s32(cfg, "rc:gop", p->gop_len ? p->gop_len : p->fps_out_num * 2);

	    /* drop frame or not when bitrate overflow */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(cfg, "rc:drop_thd", 20);        /* 20% of max bps */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_gap", 1);         /* Do not continuous drop frame */

    /* setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(cfg, "rc:bps_target", p->bps);
    switch (p->rc_mode) {
    case MPP_ENC_RC_MODE_FIXQP : {
        _log("FIXQp\n");
        /* do not setup bitrate on FIXQP mode */
    } break;
    case MPP_ENC_RC_MODE_CBR : {
        _log("CBR\n");
        /* CBR mode has narrow bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 15 / 16);
    } break;
    case MPP_ENC_RC_MODE_VBR :
    case MPP_ENC_RC_MODE_AVBR : {
        _log("AVBR \n");
        /* VBR mode has wide bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 1 / 16);
    } break;
    default : {
        /* default use CBR mode */
        _log("default");
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 15 / 16);
    } break;
    }

    /* setup qp for different codec and rc_mode */
    switch (p->type) {
    case MPP_VIDEO_CodingAVC :
    case MPP_VIDEO_CodingHEVC : {
        switch (p->rc_mode) {
        case MPP_ENC_RC_MODE_FIXQP : {
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", 20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", 20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
        } break;
        case MPP_ENC_RC_MODE_CBR :
        case MPP_ENC_RC_MODE_VBR :
        case MPP_ENC_RC_MODE_AVBR : {
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 26);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
        } break;
        default : {
            _log("unsupport encoder rc mode %d\n", p->rc_mode);
        } break;
        }
    } break;
    case MPP_VIDEO_CodingVP8 : {
        /* vp8 only setup base qp range */
        mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 40);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max",  127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min",  0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 6);
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        /* jpeg use special codec config to control qtable */
        mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", 80);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max", 99);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min", 1);
    } break;
    default : {
    } break;
    }

    /* setup codec  */
    mpp_enc_cfg_set_s32(cfg, "codec:type", p->type);
    switch (p->type) {
    case MPP_VIDEO_CodingAVC : {
        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        mpp_enc_cfg_set_s32(cfg, "h264:level", 40);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_idc", 0);
        mpp_enc_cfg_set_s32(cfg, "h264:trans8x8", 1);
    } break;
    case MPP_VIDEO_CodingHEVC :
    case MPP_VIDEO_CodingMJPEG :
    case MPP_VIDEO_CodingVP8 : {
    } break;
    default : {
        _log("unsupport encoder coding type %d\n", p->type);
    } break;
    }

    p->split_mode = 0;
    p->split_arg = 0;

    //mpp_env_get_u32("split_mode", &p->split_mode, MPP_ENC_SPLIT_NONE);
    //mpp_env_get_u32("split_arg", &p->split_arg, 0);

    if (p->split_mode) {
        _log("%p split_mode %d split_arg %d\n", ctx, p->split_mode, p->split_arg);
        mpp_enc_cfg_set_s32(cfg, "split:mode", p->split_mode);
        mpp_enc_cfg_set_s32(cfg, "split:arg", p->split_arg);
    }

    ret = mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
    if (ret) {
        _log("mpi control enc set cfg failed ret %d\n", ret);
        return ret;
    }

    /* optional */
    p->sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &p->sei_mode);
    if (ret) {
        _log("mpi control enc set sei cfg failed ret %d\n", ret);
        return ret;
    }

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        p->header_mode = MPP_ENC_HEADER_MODE_EACH_IDR;
        ret = mpi->control(ctx, MPP_ENC_SET_HEADER_MODE, &p->header_mode);
        if (ret) {
            _log("mpi control enc set header mode failed ret %d\n", ret);
            return ret;
        }
    }

    RK_U32 gop_mode = p->gop_mode;

    //mpp_env_get_u32("gop_mode", &gop_mode, gop_mode);
    if (gop_mode) {
        MppEncRefCfg ref;

        mpp_enc_ref_cfg_init(&ref);

        if (p->gop_mode < 4)
            mpi_enc_gen_ref_cfg(ref, gop_mode);
        else
            mpi_enc_gen_smart_gop_ref_cfg(ref, p->gop_len, p->vi_len);

        ret = mpi->control(ctx, MPP_ENC_SET_REF_CFG, ref);
        if (ret) {
            _log("mpi control enc set ref cfg failed ret %d\n", ret);
            return ret;
        }
        mpp_enc_ref_cfg_deinit(&ref);
    }

    /* setup test mode by env */
    //mpp_env_get_u32("osd_enable", &p->osd_enable, 0);
    //mpp_env_get_u32("osd_mode", &p->osd_mode, MPP_ENC_OSD_PLT_TYPE_DEFAULT);
    //mpp_env_get_u32("roi_enable", &p->roi_enable, 0);
    //mpp_env_get_u32("user_data_enable", &p->user_data_enable, 0);

	// RET:
	return ret;
}

void MppEncoder::setUp(int width, int height, int fps) {
	memset(&args_, 0, sizeof(MpiEncArgs));

	//计算idx是否到了gop数量，如果到了则添加一个关键帧头信息
	countIdx_ = 0;

	left = 0;
	right = 0;
	bottom = 0;
	top = 0;

	//设置的输入帧的格式信息 yuv 420p  即 I420  yyyyyyyyyyyyuuuvvv planer 结构
	//args_.format = MPP_FMT_YUV420P;
	// nv 12
	args_.format = MPP_FMT_YUV420SP;

	//设置264编码格式， AVC
	args_.type = MPP_VIDEO_CodingAVC;
	
	args_.fps_in_num = fps;
    args_.fps_out_num = fps;
	args_.width = width;
	args_.height = height;
	args_.hor_stride = MPP_ALIGN(width, 16);//mpi_enc_width_default_stride(args_.width, args_.format);
	args_.ver_stride = MPP_ALIGN(height, 16);//args_.height;
	
	packet = NULL;
	_log("in setUp");

	init();
}

}	 // namespace osee
