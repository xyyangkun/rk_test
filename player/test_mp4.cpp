/******************************************************************************
 *
 *       Filename:  hello.c
 *
 *    Description:  test 
 *
 *        Version:  1.0
 *        Created:  2023年03月01日 13时59分09秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <stdio.h>
#include <signal.h>
#include <execinfo.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <memory>
#include "Mp4Play.h"

static bool quit = false;
// 打印栈信息
//#define USE_PBT
#ifdef USE_PBT
static void print_backtrace(void)
{
#define BCACKTRACE_DEEP 100
    void *array[BCACKTRACE_DEEP];
    int size, i;
    char **strings;
    size = backtrace(array, BCACKTRACE_DEEP);
    fprintf(stderr, "\nBacktrace (%d deep):\n", size);
    strings = backtrace_symbols(array, size);
    if (strings == NULL)
    {
        fprintf(stderr, "%s: malloc error\n", __func__);
        return;
    }
    for(i = 0; i < size; i++)
    {
        fprintf(stderr, "%d: %s\n", i, strings[i]);
    }
    free(strings);
}
#endif
static void sigterm_handler(int sig) {
    fprintf(stderr, "******************signal %d, SIGUSR1 = %d\n", sig, SIGUSR1);
#ifdef USE_PBT
    //if(SIGINT != sig)
    {
        print_backtrace();
    }
#endif

    quit = true;
}

int init_vo(int hdmi_frame_rate=60)
{
    int ret;
    int enc_width = 1920;
    int enc_height = 1080;
    int mipi_screen_width = 1080;
    int mipi_screen_height = 1920;
#if 1
    VO_CHN_ATTR_S stVoAttr = {0};

    // VO[0] for primary plane
    stVoAttr.pcDevNode = "/dev/dri/card0";
    stVoAttr.emPlaneType = VO_PLANE_PRIMARY;
    stVoAttr.enImgType = IMAGE_TYPE_NV12;
    //stVoAttr.u16Zpos = 0;
    stVoAttr.u16Zpos = 3;
    stVoAttr.stDispRect.s32X = 0;
    stVoAttr.stDispRect.s32Y = 0;
    stVoAttr.stDispRect.u32Width = enc_width;//disp_width;
    stVoAttr.stDispRect.u32Height = enc_height;//disp_height;

    stVoAttr.stImgRect.u32Width = enc_width;
    stVoAttr.stImgRect.u32Height = enc_height;

    // display mode
    stVoAttr.u32Width = enc_width;//disp_width;
    stVoAttr.u32Height = enc_height;//disp_height;
    //stVoAttr.u16Fps   = 30;
    //stVoAttr.u16Fps   = 25;
    //stVoAttr.u16Fps   = 59;
    //stVoAttr.u16Fps   = 29;
    //stVoAttr.u16Fps   = 23;
    if(enc_width==1280 && enc_height == 720)
    {
        // 720p30/29/24/23 hdmi输出显示为720p60
        if(hdmi_frame_rate==30 || hdmi_frame_rate==29 || hdmi_frame_rate==24 ||hdmi_frame_rate==23)
        {
            stVoAttr.u16Fps   = 60;
        }
            // 720p25 hdmi输出显示为720p50
        else if(hdmi_frame_rate==25 )
        {
            stVoAttr.u16Fps   = 50;
        }
            // 720p50/60 不变
        else
        {
            stVoAttr.u16Fps   = hdmi_frame_rate;
        }
    }
    else
    {
        // 1080p分辨率不变
        stVoAttr.u16Fps   = hdmi_frame_rate;
    }

    //stVoAttr.u16PlaneId = 54;

    // new sdk 57
#if 1
    stVoAttr.u16PlaneId = 89;
    stVoAttr.emPlaneType = VO_PLANE_OVERLAY;

    stVoAttr.u16ConIdx = 152;
    stVoAttr.u16EncIdx = 151;

#endif
#endif


    ret = RK_MPI_VO_CreateChn(0, &stVoAttr);
    if (ret) {
        printf("Create vo[0] failed! ret=%d\n", ret);
        return -1;
    }
    {
        VO_CHN_ATTR_S stVoAttr = {0};
        ret = RK_MPI_VO_GetChnAttr(0, &stVoAttr);
        if(ret) {
            osee_error("ERROR TO get chn attr\n");
            exit(-1);
        }
        printf("===============>u16ConIdx:%d, u16EncIdx:%d, u16CrtcIdx:%d\n",
               stVoAttr.u16ConIdx, stVoAttr.u16EncIdx, stVoAttr.u16CrtcIdx);
    }

#if 1

    {
        // 只有程序退出时，才会关闭vo 1
        // VO[1] for overlay plane
        memset(&stVoAttr, 0, sizeof(stVoAttr));
        stVoAttr.pcDevNode = "/dev/dri/card0";
        stVoAttr.emPlaneType = VO_PLANE_PRIMARY;
        //stVoAttr.emPlaneType = VO_PLANE_OVERLAY;
        stVoAttr.enImgType = IMAGE_TYPE_NV12;
        stVoAttr.u16Zpos = 1;
        stVoAttr.u16Zpos = 0;
        stVoAttr.stImgRect.s32X = 0;
        stVoAttr.stImgRect.s32Y = 0;
        stVoAttr.stImgRect.u32Width  = enc_height;//mipi_screen_width;
        stVoAttr.stImgRect.u32Height = enc_width;//mipi_screen_height;

        stVoAttr.stDispRect.s32X = 0;
        stVoAttr.stDispRect.s32Y = 0;
        stVoAttr.stDispRect.u32Width  = mipi_screen_width;
        stVoAttr.stDispRect.u32Height = mipi_screen_height;

        stVoAttr.u32Width = mipi_screen_width;
        stVoAttr.u32Height = mipi_screen_height;
        stVoAttr.u16Fps   = 60;

        // new sdk
#if 1
        stVoAttr.u16PlaneId = 103;
        //stVoAttr.u16PlaneId = 73;
        stVoAttr.emPlaneType = VO_PLANE_OVERLAY;
        stVoAttr.u16ConIdx = 163;
        stVoAttr.u16EncIdx = 162;
        //stVoAttr.u16CrtcIdx = 87;
#endif


        ret = RK_MPI_VO_CreateChn(1, &stVoAttr);
        if (ret) {
            printf("Create vo[1] failed! ret=%d\n", ret);
            return -1;
        }
    }
#endif
    return 0;
}

int deinit_vo()
{
    RK_MPI_VO_DestroyChn(0);
    {
        // 只有程序退出时，才会关闭vo 1
        RK_MPI_VO_DestroyChn(1);
    }

    return 0;
}

zlog_category_t * log_category = NULL;
#define zlog_path "/userdata/osee_live/zlog.conf"

int main(int argc, char *argv[])
{
    // 将配置文件复制到 /userdata下面
    system("mkdir /userdata/osee_live/");
    system("cp /oem/osee_live/zlog.conf /userdata/osee_live/");
    if(zlog_init(zlog_path)) {
        printf("ERROR in zlog_init\n");
        zlog_fini();
        exit(1);
    }
    log_category = zlog_get_category("zlog");
    if(!log_category)
    {
        fprintf(stderr, "============================> Error in get comm protcol category\n");
        zlog_fini();
    }


    printf("%s initial finish\n", __func__);
    signal(SIGINT, sigterm_handler);
#ifdef USE_PBT
    signal(SIGILL, sigterm_handler);    /* Illegal instruction.  */
    signal(SIGABRT, sigterm_handler);   /* Abnormal termination.  */
    signal(SIGFPE, sigterm_handler);    /* Erroneous arithmetic operation.  */
    signal(SIGSEGV, sigterm_handler);   /* Invalid access to storage.  */
    signal(SIGTERM, sigterm_handler);   /* Termination request.  */
    /* Historical signals specified by POSIX. */
    signal(SIGBUS, sigterm_handler);    /* Bus error.  */
    signal(SIGSYS, sigterm_handler);    /* Bad system call.  */
    signal(SIGPIPE, sigterm_handler);   /* Broken pipe.  */
    signal(SIGALRM, sigterm_handler);   /* Alarm clock.  */
    /* New(er) POSIX signals (1003.1-2008, 1003.1-2013).  */
    signal(SIGXFSZ, sigterm_handler);   /* File size limit exceeded.  */
#endif
    init_vo();
    std::shared_ptr<Mp4Play> mp4;
    mp4.reset(new Mp4Play, [](Mp4Play *p){delete p;});
    const char *path = "test.mp4";
    mp4->Mp4Open(path);

    while (!quit) {
        usleep(500000);
    }

    deinit_vo();
    printf("%s %d===================>\n", __FUNCTION__, __LINE__);
    return 0;
}
