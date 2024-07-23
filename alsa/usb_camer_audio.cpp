//
// Created by win10 on 2024/7/23.
//

#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include "usb_camer_audio.h"
#include "usb_camera_get.h"
#include "usb_camera_notify.h"
#include "sound_card_get.h"
#include "alsa.h"


int open_sound_card(char *name, int samplerate, int channels, snd_pcm_format_t format, snd_pcm_t **handle, bool is_write, bool is_block = true);
int close_sound_card(snd_pcm_t **handle);

static snd_pcm_t * usb_in_handle;
static int read_usb_sound_card_quit = false;
// 读声卡线程
pthread_t read_usb_sound_card_th = 0;




// 读取声卡线程
void *read_usb_in_sound_card_proc(void *param)
{

    int err = 0;
    int queue_read_size = sizeof(int16_t) * 2 * AUDIO_FRAME_SIZE;

    char *buf = (char *)malloc(AUDIO_FRAME_SIZE * 20 * 2);

    snd_pcm_status_t *usb_in_status;
    snd_pcm_status_alloca(&usb_in_status);

    printf("AUDIO_FRAME_SIZE = %d\n", AUDIO_FRAME_SIZE);

#define POINT_SIZE ( 2 * sizeof(int16_t))

    while(!read_usb_sound_card_quit) {
        {
            // 获取状态
            int err = snd_pcm_status(usb_in_handle, usb_in_status);
            if (err < 0) {
                printf("无法获取 PCM 状态: %s\n", snd_strerror(err));
                // 无法获取PCM状态时，就是拔下卡了
                break;
            }
            snd_pcm_state_t state = snd_pcm_status_get_state(  usb_in_status);
            snd_pcm_uframes_t avail = snd_pcm_status_get_avail(usb_in_status);
            snd_pcm_uframes_t delay = snd_pcm_status_get_delay(usb_in_status);

            printf(">>>>>>>>>>>>>>>>> delay:%d avail:%d state=%d \n", delay, avail, state);

            // 读usb_in声卡
            if ((err = snd_pcm_readi(usb_in_handle, buf, AUDIO_FRAME_SIZE)) != AUDIO_FRAME_SIZE) {
                printf("%d read from audio interface failed (%s)", err, snd_strerror (err));
                if (err == -EPIPE) {
                    // 没有及时取走数据
                    /* EPIPE means overrun */
                    printf("underrun overrun\n");
                    snd_pcm_prepare(usb_in_handle);
                } else if (err < 0) {
                    printf("error from writei: %s\n", snd_strerror(err));
                }
                // 不进行退出处理
                // exit (1);
                usleep(10*1000);
            } else
            {
                printf("read usb audio card size:%d\n", err);
            }

        }


    }
    close_sound_card(&usb_in_handle);
    usb_in_handle = nullptr;
    printf("debug %s %d  read usb sound card proc will exit!\n", __func__, __LINE__);

#if TEST_TIME==1
    time_diff_delete(&hd);
#endif
    //snd_pcm_status_free(usb_in_status);

    free(buf);
    return nullptr;
}


int init_usb_audio()
{
    // usb 声卡如果不存在，打开失败自动退出
    int ret;

    // 动态获取usb 配置
    sound_card_info info;
    char dev_name[100]= {0};
    if(0 != found_sound_card(dev_name))
    {
        printf("not found sound card\n");
        return -1;
    }

    // 获取声卡支持的配置
    if(0 != get_sound_card_info(dev_name, &info))
    {
        printf("not support sound card or sound card not exist\n");
        return -1;
    }

    printf("usb audio card name:%s sample_rate:%d channels:%d format:%d\n", dev_name, info.sample, info.channels, info.format);

    snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
    // 打开usb_in声卡读capture
    ret = open_sound_card(dev_name, info.sample, info.channels,
                          (snd_pcm_format_t)format, &usb_in_handle, false);
    if(ret != 0) {
        printf("error in read open sound card!");
        return ret;
    }

    read_usb_sound_card_quit = false;
    ret = pthread_create(&read_usb_sound_card_th, NULL, read_usb_in_sound_card_proc, NULL);
    if (ret != 0) {
        printf("error to create th:%s", strerror(errno));
        exit(1);
    }

    return 0;
}

int deinit_usb_audio()
{
    int ret;
    printf("debug %s %d will wait\n", __func__, __LINE__);
    if(!read_usb_sound_card_th)
    {
        read_usb_sound_card_quit = true;
        pthread_join(read_usb_sound_card_th, NULL);
        read_usb_sound_card_th = 0;
    }
    printf("debug %s %d will exit\n", __func__, __LINE__);
    return 0;
}


static void usb_camera_notify(char *path, int cb_type, unsigned int resolution, unsigned int fps, long handle)
{
    int w,h;
    printf("notify usb camer path:%s cb_type:%d resolution:%d  handle:%ld, fps:%d\n", path, cb_type, resolution, handle, fps);
    if(handle == 0)
    {
        printf("ERROR IN usb camera notify handle =0!!\n");
        exit(1);
    }

    // 调用 usb camera 断开，关闭

    // 插入
    if(cb_type == 1)
    {
        // 插入时， p_usb_camera肯定为空
        //assert(obj->p_usb_camera == nullptr);

        // 根据获取的分辨率打开camera
        if(resolution == 1)
        {
            w = 1920;
            h = 1080;
        }else  if(resolution == 2)
        {
            w = 1280;
            h = 720;
        }else {
            printf("ERROR not support usb camera\n");
            //exit(1);
            return ;
        }

        //obj->start_usb_camera(path, w, h, fps);
        init_usb_audio();
    }

    // 拔下
    if(cb_type == 2)
    {
        //拔下 时， p_usb_camera肯定不为空？？？ 至少目前是这样
        //assert(obj->p_usb_camera != nullptr);
        // obj->stop_usb_camera();
        deinit_usb_audio();

    }
}


static void* usb_notify_hd=NULL;

int init_usb_camera()
{
    int ret = -1;
    int usb_ret = -1;
    t_camera_info info = {0};
    // 搜索是否有 usb camera
    {
        const char *path[] = {
                "/dev/video9",
                "/dev/video10",
                "/dev/video11",
                "/dev/video12",
                "/dev/video13",
                "/dev/video14",
        };
        int size = sizeof(path)/sizeof(char *);
        usb_ret = get_camera_info((char **)path, size, &info);
    }
    int w ,h;
    // 有usb 打开camera
    if(usb_ret == 0)
    {
        // 根据获取的分辨率打开camera
        if(info.resolution == 1)
        {
            w = 1920;
            h = 1080;
        }else  if(info.resolution == 2)
        {
            w = 1280;
            h = 720;
        }else {
            printf("ERROR not support usb camera\n");
            exit(1);
        }
        printf("usb camera w:%d h:%d fps:%d\n", w, h, info.frame_rate);

        //start_usb_camera(info.path, w, h, info.frame_rate);
        init_usb_audio();
    }

    //  打开 usb camera 检测程序
    // no usb camera
    //if(0)
    {
        if(usb_ret == 0)
        {
            // 程序打开时, usb camera已经存在
            // usb_notify_hd =
            ret = init_usb_camera_notify_v1(&usb_notify_hd, usb_camera_notify, (long)1, info.path);
            if(ret != 0)
            {
                printf("ERROR in usb notify\n");
                exit(1);
            }
        }
        else
        {
            // 程序打开时, usb camera不存在, 不传递 usb camera路径
            ret = init_usb_camera_notify_v1(&usb_notify_hd, usb_camera_notify, (long)1, nullptr);
            if(ret != 0)
            {
                printf("ERROR in usb notify\n");
                exit(1);
            }
        }
    }

    printf("debug %s %d, usb_ret=%d\n", __func__, __LINE__, usb_ret);

    return usb_ret;
}

int deinit_usb_camera()
{
    int  ret;

    ret = deinit_usb_camera_notify(&usb_notify_hd);
    if(ret != 0)
    {
        printf("ERROR in delete usb camera notify thread\n");
        exit(1);
    }

    printf("debug %s %d\n", __func__, __LINE__);

    // stop_usb_camera();

    deinit_usb_audio();
    return 0;
}