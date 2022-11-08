/******************************************************************************
 *
 *       Filename:  rk_h264_decode.c
 *
 *    Description:  rk h264 decode
 *    参考：https://github.com/MUZLATAN/ffmpeg_rtsp_mpp/blob/master/MppDecode.cpp
 *
 *        Version:  1.0
 *        Created:  2022年10月09日 17时06分58秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include "rk_h264_decode.h"
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

int vpu_decode_h264_init(struct vpu_h264_decode* decode, int width, int height)
{
    int ret;
    decode->in_width = width;
    decode->in_height = height;

	/*
    ret = mpp_buffer_group_get_internal(&decode->memGroup, MPP_BUFFER_TYPE_ION);
    if (MPP_OK != ret) {
        printf("memGroup mpp_buffer_group_get failed\n");
		exit(1);
        return ret;
    }
	*/

    ret = mpp_create(&decode->mpp_ctx, &decode->mpi);
    if (MPP_OK != ret) {
        printf("mpp_create failed\n");
		exit(1);
        return -1;
    }

	MppCodingType type  = MPP_VIDEO_CodingAVC;
	//MppCodingType type  = MPP_VIDEO_CodingHEVC;
    ret = mpp_init(decode->mpp_ctx, MPP_CTX_DEC, type);
    if (MPP_OK != ret) {
        printf("mpp_init failed\n");
		exit(1);
        return -1;
    }

    MppApi* mpi = decode->mpi;
    MppCtx mpp_ctx = decode->mpp_ctx;
	MppParam param      = NULL;
	RK_U32 need_split   = 1;
	MpiCmd mpi_cmd      = MPP_CMD_BASE;
	mpi_cmd = MPP_DEC_SET_PARSER_SPLIT_MODE;
	param = &need_split;
	ret = mpi->control(mpp_ctx, mpi_cmd, param);
	if (MPP_OK != ret) {
		printf("mpi->control failed\n");
		//deInit(&packet, &frame, ctx, buf, data);
		exit(1);
        return MPP_ERR_NOMEM;
	}

	mpi_cmd = MPP_SET_INPUT_BLOCK;
	param = &need_split;
	ret = mpi->control(mpp_ctx, mpi_cmd, param);
	if (MPP_OK != ret) {
		printf("mpi->control failed\n");
		//deInit(&packet, &frame, ctx, buf, data);
		exit(1);
        return MPP_ERR_NOMEM;
	}

    MppFrame frame;
    ret = mpp_frame_init(&frame);
    if (!frame || (MPP_OK != ret)) {
        printf("failed to init mpp frame!");
		exit(1);
        return MPP_ERR_NOMEM;
    }

	RK_U32 fbc_en = 1;
	MppFrameFormat format = MPP_FMT_YUV420SP;
	mpp_env_set_u32("fbc_dec_en",  1);
	mpp_env_get_u32("fbc_dec_en", &fbc_en, 0);
	
	if (fbc_en)
	{
		format = format | MPP_FRAME_FBC_AFBC_V2;
	}
	printf("yk debug decode fbc encode %d\n", fbc_en);

	// yk add 
#if 1
    ret = mpi->control(mpp_ctx, MPP_DEC_SET_OUTPUT_FORMAT, MPP_FMT_YUV420SP/*(MppParam)format*/);
#else
    ret = mpi->control(mpp_ctx, MPP_DEC_SET_OUTPUT_FORMAT, /*MPP_FMT_YUV420SP*/(MppParam)format);
#endif
    if (MPP_OK != ret) {
		printf("Failed to set output format (ret = %d)\n", ret);
		return -3;
	}

	printf("===============> decode init over!\n");
    return 0;
}

void dump_mpp_frame_to_file(MppFrame frame, FILE *fp)
{
    RK_U32 width    = 0;
    RK_U32 height   = 0;
    RK_U32 h_stride = 0;
    RK_U32 v_stride = 0;

    MppBuffer buffer    = NULL;
    RK_U8 *base = NULL;

    width    = mpp_frame_get_width(frame);
    height   = mpp_frame_get_height(frame);
    h_stride = mpp_frame_get_hor_stride(frame);
    v_stride = mpp_frame_get_ver_stride(frame);
    buffer   = mpp_frame_get_buffer(frame);

    base = (RK_U8 *)mpp_buffer_get_ptr(buffer);
    RK_U32 buf_size = mpp_frame_get_buf_size(frame);
    size_t base_length = mpp_buffer_get_size(buffer);
    //mpp_log("base_length = %d\n",base_length);

    RK_U32 i;
    RK_U8 *base_y = base;
    RK_U8 *base_c = base + h_stride * v_stride;

    //保存为YUV420sp格式 nv12
#if 1
    for (i = 0; i < height; i++, base_y += h_stride)
    {
        fwrite(base_y, 1, width, fp);
    }
    for (i = 0; i < height / 2; i++, base_c += h_stride)
    {
        fwrite(base_c, 1, width, fp);
    }
#endif

    //保存为YUV420p格式 i420
#if 0
    for(i = 0; i < height; i++, base_y += h_stride)
    {
        fwrite(base_y, 1, width, fp);
    }
    for(i = 0; i < height * width / 2; i+=2)
    {
        fwrite((base_c + i), 1, 1, fp);
    }
    for(i = 1; i < height * width / 2; i+=2)
    {
        fwrite((base_c + i), 1, 1, fp);
    }
#endif
}


int vpu_decode_h264_doing(struct vpu_h264_decode* decode, void* in_data, RK_S32 in_size,
                          int out_fd, void* out_data, int *dtype)
{
	RK_U32 pkt_done = 0;
	RK_U32 pkt_eos  = 0;
	RK_U32 err_info = 0;
	MPP_RET ret = MPP_OK;
	MppCtx ctx  = decode->mpp_ctx;
	MppApi *mpi = decode->mpi;
	MppPacket packet = NULL;
	MppFrame  frame  = NULL;
	size_t read_size = 0;


	ret = mpp_packet_init(&packet, in_data, in_size);
	mpp_packet_set_pts(packet, 0); // todo 要设置时间戳

	do {
		RK_S32 times = 5;
		// send the packet first if packet is not done
		if (!pkt_done) {
			ret = mpi->decode_put_packet(ctx, packet);
			if (MPP_OK == ret)
			{
				pkt_done = 1;
			}
		}

		// then get all available frame and release
		do {
			RK_S32 get_frm = 0;
			RK_U32 frm_eos = 0;

try_again:
			ret = mpi->decode_get_frame(ctx, &frame);
			if (MPP_ERR_TIMEOUT == ret) {
				if (times > 0) {
					times--;
					//msleep(2);
					usleep(2000);
					goto try_again;
				}
				printf("decode_get_frame failed too much time\n");
			}
			if (MPP_OK != ret) {
				printf("decode_get_frame failed ret %d\n", ret);
				break;
			}

			if (frame) {
				if (mpp_frame_get_info_change(frame)) {
					RK_U32 width = mpp_frame_get_width(frame);
					RK_U32 height = mpp_frame_get_height(frame);
					RK_U32 hor_stride = mpp_frame_get_hor_stride(frame);
					RK_U32 ver_stride = mpp_frame_get_ver_stride(frame);
					RK_U32 buf_size = mpp_frame_get_buf_size(frame);

					printf("decode_get_frame get info changed found\n");
					printf("decoder require buffer w:h [%d:%d] stride [%d:%d] buf_size %d",
							width, height, hor_stride, ver_stride, buf_size);

					ret = mpp_buffer_group_get_internal(&decode->frm_grp, MPP_BUFFER_TYPE_ION);
					if (ret) {
						printf("get mpp buffer group  failed ret %d\n", ret);
						break;
					}
					mpi->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, decode->frm_grp);

					mpi->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
				} else {
					err_info = mpp_frame_get_errinfo(frame) | mpp_frame_get_discard(frame);
					if (err_info) {
						printf("decoder_get_frame get err info:%d discard:%d.\n",
								mpp_frame_get_errinfo(frame), mpp_frame_get_discard(frame));
					}
					decode->frame_count++;
					printf("decode_get_frame get frame %d\n", decode->frame_count);

					MppBuffer buffer    = NULL;
					buffer   = mpp_frame_get_buffer(frame);

					printf("get buf=%p, len=%d\n", mpp_buffer_get_ptr(buffer), mpp_frame_get_buf_size(frame));
					if (decode->fp_output && !err_info){
						//cv::Mat rgbImg;
						//YUV420SP2Mat(frame, rgbImg);
						//    cv::imwrite("./"+std::to_string(count++)+".jpg", rgbImg);

						dump_mpp_frame_to_file(frame, decode->fp_output);
					}
				}
				frm_eos = mpp_frame_get_eos(frame);
				mpp_frame_deinit(&frame);

				frame = NULL;
				get_frm = 1;
			}

			// try get runtime frame memory usage
			if (decode->frm_grp) {
				size_t usage = mpp_buffer_group_usage(decode->frm_grp);
				if (usage > decode->max_usage)
					decode->max_usage = usage;
			}

			// if last packet is send but last frame is not found continue
			if (pkt_eos && pkt_done && !frm_eos) {
				//msleep(10);
				usleep(10*1000);
				continue;
			}

			if (frm_eos) {
				printf("found last frame\n");
				break;
			}

			if (decode->frame_num > 0 && decode->frame_count >= decode->frame_num) {
				decode->eos = 1;
				break;
			}

			if (get_frm)
				continue;
			break;
		} while (1);

		if (decode->frame_num > 0 && decode->frame_count >= decode->frame_num) {
			decode->eos = 1;
			printf("reach max frame number %d\n", decode->frame_count);
			break;
		}

		if (pkt_done)
			break;

		/*
		 * why sleep here:
		 * mpi->decode_put_packet will failed when packet in internal queue is
		 * full,waiting the package is consumed .Usually hardware decode one
		 * frame which resolution is 1080p needs 2 ms,so here we sleep 3ms
		 * * is enough.
		 */
		//msleep(3);
		usleep(3*1000);
	} while (1);
	mpp_packet_deinit(&packet);

	return ret;

}

int vpu_decode_h264_done(struct vpu_h264_decode* decode)
{
    MPP_RET ret = MPP_OK;

    MppCtx mpp_ctx = decode->mpp_ctx;
    MppApi* mpi = decode->mpi;
	ret = mpi->reset(mpp_ctx);
	if(ret) {
		printf("!!!!============> mpi->reset failed %p, ret=%d\n", decode->mpp_ctx, ret);
		exit(-1);
	}

    ret = mpp_destroy(decode->mpp_ctx);
    if (ret != MPP_OK) {
        printf("something wrong with mpp_destroy! ret:%d\n", ret);
		exit(-1);
    }

    if (decode->frm_grp) {
        mpp_buffer_group_put(decode->frm_grp);
        decode->frm_grp = NULL;
    }
    if (decode->pkt_grp) {
        mpp_buffer_group_put(decode->pkt_grp);
        decode->pkt_grp = NULL;
    }

    return ret;
}
