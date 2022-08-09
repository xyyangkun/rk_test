//
// Created by win10 on 2022/8/9.
// yuv to rgb
//  720prgb copy 1080rgba

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <rga/im2d.hpp>


#include <rga/RgaApi.h>
//#include "RgaApi.h"
#include "rkRgaApi.h"
#include "rkmedia/rkmedia_api.h"

#define ALIGN16(value) ((value + 15) & (~15))


static void blend_buffer00(void *src_p, int src_fd, int src_w, int src_h, int src_fmt,
                           void *dst_p, int dst_fd, int dst_w, int dst_h, int dst_fmt,
                           void *src1_p, int src1_fd, int src1_w, int src1_h, int src1_fmt)
{
    int ret;
    rga_buffer_t    src;
    rga_buffer_t    src1;
    rga_buffer_t    dst;

    im_rect         src_rect = {0,0,src_w, src_h};
    im_rect         src1_rect = {0, 0, src1_w, src1_h};
    im_rect         dst_rect   = {0, 0, dst_w, dst_h};

    //memset(&src_rect, 0, sizeof(src_rect));
    //memset(&src1_rect, 0, sizeof(src_rect));
    //memset(&dst_rect, 0, sizeof(dst_rect));

    memset(&src, 0, sizeof(src));
    memset(&src1, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));

#if 0
    src = wrapbuffer_physicaladdr(src_p, src_w, src_h, src_fmt);
	src1 = wrapbuffer_physicaladdr(src1_p, src1_w, src1_h, src1_fmt);
	dst = wrapbuffer_physicaladdr(dst_p, dst_w, dst_h, dst_fmt);
#else
    src =  wrapbuffer_fd(src_fd, src_w, src_h, src_fmt);
    src1 = wrapbuffer_fd(src1_fd, src1_w, src1_h, src1_fmt);
    dst =  wrapbuffer_fd(dst_fd, dst_w, dst_h, dst_fmt);
#endif

    src.global_alpha=0xcc;
    src1.global_alpha=0x11;

    src_rect.x = 100;
    src_rect.y = 200;
    src_rect.width = src1_w;
    src_rect.height = src1_h;

    src1_rect.x = 0;
    src1_rect.y = 0;
    src1_rect.width = src_rect.width;
    src1_rect.height = src_rect.height;

#if 0
    dst_rect.x = src_rect.x;
    dst_rect.y = src_rect.y;
    dst_rect.width = src_rect.width;
    dst_rect.height = src_rect.height;
#else
    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = dst_w;
    dst_rect.height = dst_h;
#endif

    ret = imcheck_composite(src, dst, src1, src_rect, dst_rect, src1_rect);
    if (IM_STATUS_NOERROR != ret) {
        printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
        exit(1);
    }

    int usage = IM_SYNC | IM_ALPHA_BLEND_DST_OVER;

    //ret = improcess(src, dst, src1, src_rect, dst_rect, src1_rect, -1, NULL, NULL, usage);
    ret = improcess(src, dst, src1, src_rect, dst_rect, src1_rect, usage);
    printf("==>.... %s\n", imStrError(ret));
    if (ret != IM_STATUS_SUCCESS)
        exit(1);




}



static void blend_buffer(void *src_p, int src_fd, int src_w, int src_h, int src_fmt,
                         void *dst_p, int dst_fd, int dst_w, int dst_h, int dst_fmt,
                         void *src1_p, int src1_fd, int src1_w, int src1_h, int src1_fmt)
{
    int ret;
    rga_buffer_t    src;
    rga_buffer_t    src1;
    rga_buffer_t    dst;

    im_rect         src_rect = {0,0,src_w, src_h};
    im_rect         src1_rect = {0, 0, src1_w, src1_h};
    im_rect         dst_rect   = {0, 0, dst_w, dst_h};

    //memset(&src_rect, 0, sizeof(src_rect));
    //memset(&src1_rect, 0, sizeof(src_rect));
    //memset(&dst_rect, 0, sizeof(dst_rect));

    memset(&src, 0, sizeof(src));
    memset(&src1, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));

#if 0
    src = wrapbuffer_physicaladdr(src_p, src_w, src_h, src_fmt);
	src1 = wrapbuffer_physicaladdr(src1_p, src1_w, src1_h, src1_fmt);
	dst = wrapbuffer_physicaladdr(dst_p, dst_w, dst_h, dst_fmt);
#else
    src =  wrapbuffer_fd(src_fd, src_w, src_h, src_fmt);
    src1 = wrapbuffer_fd(src1_fd, src1_w, src1_h, src1_fmt);
    dst =  wrapbuffer_fd(dst_fd, dst_w, dst_h, dst_fmt);
#endif

    src.global_alpha=0xcc;
    src1.global_alpha=0x11;
#if 1
    // ret = imcheck_composite(src, dst, src1, src_rect, dst_rect, src1_rect, IM_ALPHA_BLEND_DST_OVER);
    ret = imcheck_composite(src, dst, src1, src_rect, dst_rect, src1_rect);
    if (IM_STATUS_NOERROR != ret) {
        printf("11error in check\n");
        exit(1);
    }
#endif
    //ret = imcomposite(src, src1, dst, IM_ALPHA_BLEND_DST_OVER);
    ret = imcomposite(src, src1, dst, IM_ALPHA_BLEND_DST_OVER);
    //ret = imcomposite(src, src1, src, IM_ALPHA_BLEND_DST_OVER);
    if (IM_STATUS_SUCCESS != ret) {
        printf("22error in check:%d\n", ret);
        exit(1);
    }
}


static void compose_buffer(void *src1_p, int src1_w, int src1_h, int src1_fmt,
                           void *src2_p, int src2_w, int src2_h, int src2_fmt,
                           void *dst_p, int dst_w, int dst_h, int dst_fmt,
                           int x, int y, int w, int h) {
    rga_info_t src1, src2, dst;
    memset(&src1, 0, sizeof(rga_info_t));
    src1.fd = -1;
    src1.virAddr = src1_p;
    src1.mmuFlag = 1;
    //src1.blend = 0x000100;
    src1.blend = 0xcc0405;
    //rga_set_rect(&src1.rect, 0, 0, src1_w, src1_h, ALIGN16(src1_w), ALIGN16(src1_h), src1_fmt);
    rga_set_rect(&src1.rect, 0, 0, src1_w, src1_h, src1_w, src1_h, src1_fmt);

    memset(&src2, 0, sizeof(rga_info_t));
    src2.fd = -1;
    src2.virAddr = src2_p;
    src2.mmuFlag = 1;
    //rga_set_rect(&src2.rect, 0, 0, src2_w, src2_h, ALIGN16(src2_w), ALIGN16(src2_h), src2_fmt);
    rga_set_rect(&src2.rect, 0, 0, src2_w, src2_h, src2_w, src2_h, src2_fmt);
    //src2.blend = 0x110405;
    //src2.blend = 0xcc0002;

    memset(&dst, 0, sizeof(rga_info_t));
    dst.fd = -1;
    dst.virAddr = dst_p;
    dst.mmuFlag = 1;
    //dst.blend = 0xcc0405;
    //rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, ALIGN16(dst_w), ALIGN16(dst_h), dst_fmt);
    rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dst_fmt);
    if (c_RkRgaBlit(&src1, &dst, &src2))
        printf("%s: rga fail\n", __func__);
}

static void copy_rgb(void *src_p, int src_w, int src_h, int src_w1, int src_h1,  int src_fmt,
                            void *dst_p, int dst_w, int dst_h, int dst_fmt,
                            int rotation) {
    printf("%d %d %d %d\n", src_w, src_h, dst_w, dst_h);
    rga_info_t src, dst;
    memset(&src, 0, sizeof(rga_info_t));
    src.fd = -1;
    src.virAddr = src_p;
    src.mmuFlag = 1;
    src.rotation = rotation;
    //rga_set_rect(&src.rect, 0, 0, src_w, src_h, ALIGN16(src_w), ALIGN16(src_h), src_fmt);
    rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w1, src_h1, src_fmt);
    memset(&dst, 0, sizeof(rga_info_t));
    dst.fd = -1;
    dst.virAddr = dst_p;
    dst.mmuFlag = 1;
    //rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, ALIGN16(dst_w), ALIGN16(dst_w), dst_fmt);
    //rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_w, dst_fmt);
    printf("%d %d %d %d\n", src_w, src_h, dst_w, dst_h);
    //rga_set_rect(&dst.rect, 0, 0, src_w, src_h, src_w, src_h, dst_fmt);
    rga_set_rect(&dst.rect, 110, 110, src_w, src_h, dst_w, dst_h, dst_fmt);
    if (c_RkRgaBlit(&src, &dst, NULL))
        printf("%s: rga fail\n", __func__);
}

int main()
{
    unsigned int src1_w, src1_h;
    unsigned int src2_w, src2_h;
    unsigned int dst_w,  dst_h ;
    IMAGE_TYPE_E src1_type, src2_type, dst_type;
    MEDIA_BUFFER mb_src1, mb_src2, mb_dst;

    src1_w = 1920;
    src1_h = 1080;
    src1_type = IMAGE_TYPE_ARGB8888;

    src2_w = 1280;
    src2_h = 720;
    src2_type = IMAGE_TYPE_ARGB8888;//IMAGE_TYPE_ARGB8888;//RK_FORMAT_RGBA_8888;

    dst_w = 1920;
    dst_h = 1080;
    dst_type = IMAGE_TYPE_ARGB8888;
    //dst_type = IMAGE_TYPE_ARGB8888;
    // 分配内存
    {
        MB_IMAGE_INFO_S disp_info= {src1_w, src1_h,
                                    src1_w, src1_h,
                                    src1_type};

        mb_src1 = RK_MPI_MB_CreateImageBuffer(&disp_info, RK_TRUE, 0);
        if (!mb_src1) {
            printf("ERROR: no space left!, src1_mb\n");
            exit(1);
        }
    }
    {
        MB_IMAGE_INFO_S disp_info= {src2_w, src2_h,
                                    src2_w, src2_h,
                                    src2_type};

        mb_src2 = RK_MPI_MB_CreateImageBuffer(&disp_info, RK_TRUE, 0);
        if (!mb_src2) {
            printf("ERROR: no space left!, src2_mb\n");
            exit(1);
        }
    }


    {
        MB_IMAGE_INFO_S disp_info= {dst_w, dst_h,
                                    dst_w, dst_h,
                                    dst_type};

        mb_dst = RK_MPI_MB_CreateImageBuffer(&disp_info, RK_TRUE, 0);
        if (!mb_dst) {
            printf("ERROR: no space left!, dst_mb\n");
            exit(1);
        }
    }

    // 读取数据
    if(0)
    {
        // src1
        const char *src1 = "1080p_nv12.yuv";
        FILE *fp = fopen(src1, "rb");
        assert(fp);
        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);
        printf("src1 file size:%d\n", size);
        fseek(fp, 0, SEEK_SET);
        assert(size == src1_w*src1_h*3/2);
        int ret = fread(RK_MPI_MB_GetPtr(mb_src1), 1, size, fp);
        assert(ret == size);
        fclose(fp);
    }

    {
        // src2
        const char *src2 = "720p_rk.rgb";
        FILE *fp = fopen(src2, "rb");
        assert(fp);
        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);
        printf("src2 file size:%d\n", size);
        fseek(fp, 0, SEEK_SET);
        assert(size == src2_w*src2_h*4);
        int ret = fread(RK_MPI_MB_GetPtr(mb_src2), 1, size, fp);
        assert(ret == size);
        fclose(fp);
    }


#if 1
    // 调用
    copy_rgb(RK_MPI_MB_GetPtr(mb_src2), src2_w, src2_h, src2_w, src2_h, RK_FORMAT_RGBA_8888,
                    RK_MPI_MB_GetPtr(mb_dst), dst_w, dst_h, RK_FORMAT_RGBA_8888,0);
            //RK_MPI_MB_GetPtr(mb_dst), dst_w, dst_h, RK_FORMAT_RGBA_8888,0);
#else
    compose_buffer(
            RK_MPI_MB_GetPtr(mb_src1), src1_w, src1_h,  RK_FORMAT_YCbCr_420_SP,
            //RK_MPI_MB_GetPtr(mb_src2), src2_w, src2_h, RK_FORMAT_BGRA_8888,
            RK_MPI_MB_GetPtr(mb_src2), src2_w, src2_h, RK_FORMAT_RGB_888,
            RK_MPI_MB_GetPtr(mb_dst), dst_w, dst_h, RK_FORMAT_YCbCr_420_SP,
            0, 0, src2_w, src2_w);
#endif


    // 将结果写入内存
    {
        // dst
        const char *dst = "dst.rgb";
        FILE *fp = fopen(dst, "wb");
        assert(fp);
        int size = dst_w*dst_h*4;
        int ret = fwrite(RK_MPI_MB_GetPtr(mb_dst), 1, size, fp);
        assert(ret == size);
        fclose(fp);
    }
    if(0)
    {
        // src1
        const char *dst = "dst1.yuv";
        FILE *fp = fopen(dst, "wb");
        assert(fp);
        int size = dst_w*src2_h*4;
        int ret = fwrite(RK_MPI_MB_GetPtr(mb_src2), 1, size, fp);
        assert(ret == size);
        fclose(fp);
    }


    return 0;
}
