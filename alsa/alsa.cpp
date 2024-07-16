//
// Created by win10 on 2024/7/15.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <map>
#include <unordered_map>

#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <thread>
#include <mutex>
#include <cmath>
#include "UnlockQueue.h"

#include "alsa_log.h"
#include "osee_error.h"
#include "alsa.h"
#include "time_utils.h"


#define VERSION "0.2"
// -1 close  关闭日志
// 0 error  default，不想输出太多调试信息
// 1 info   输出info信息
// 2 dbg    输出调试信息
static int dbg_flag = 1;
/// \brief 初始化是否要运行测试
/// \param void
/// \return void
static void _dbg_init(void)
{
    const char *env_dbg    = getenv("dbg");
    if ( env_dbg )
    {
        // export dbg=1  or export mp4_writer=2
        if(env_dbg)dbg_flag = atoi(env_dbg);
    }
}

#define dbg(fmt, args...)\
    do {\
        if (dbg_flag>=2){debug_log_message("[%s %d]" fmt, __FUNCTION__, __LINE__, ##args);}\
	}\
    while(0)

#define error(fmt, args...)\
    do {\
        {if (dbg_flag>=0)error_log_message("[%s %d]" fmt, __FUNCTION__, __LINE__, ##args);}\
	}\
    while(0)

#define info(fmt, args...)\
    do {\
        {if (dbg_flag>=1)info_log_message("[%s %d]" fmt, __FUNCTION__, __LINE__, ##args);}\
	}\
    while(0)


//输出算法耗时时间us
#define TEST_TIME 1

// 0 不设置音频读取线程优先级
// 1 设置音频读取线程优先级
#define SET_THREAD_PRIORITY 1

#if SET_THREAD_PRIORITY == 1
static pthread_attr_t attr_read_thread;
#endif

// line_in -> line_out 20
// hdmi_in -> line_out 64
// usb_in  -> hdmi_out 64
// line_in -> hdmi_out 64
// hdmi_in -> hdmi_out 64
// usb_in  -> hdmi_out 64

#define AUDIO_FRAME_SIZE 160
#define AUDIO_READ_CHN  2  // 2 4
#define AUDIO_WRITE_CHN 2  // 4 6


#define LSX_USE_VAR(x)  ((void)(x)) /* Parameter or variable is intentionally unused. */
#define LSX_UNUSED  __attribute__ ((unused)) /* Parameter or local variable is intentionally unused. */
#define PCM_LOCALS int16_t sox_macro_temp_pcm LSX_UNUSED; \
  double sox_macro_temp_double LSX_UNUSED

#define INT16_TO_FLOAT(d) ((d)*(1.0 / (INT16_MAX + 1.0)))

#define FLOAT_TO_PCM(d) FLOAT_64BIT_TO_PCM(d)

#define FLOAT_64BIT_TO_PCM(d)                        \
  (int16_t)(                                                  \
    LSX_USE_VAR(sox_macro_temp_pcm),                            \
     sox_macro_temp_double = (d) * (INT16_MAX + 1.0),       \
    sox_macro_temp_double < 0 ?                                 \
      sox_macro_temp_double <= INT16_MIN - 0.5 ?           \
        /*++(clips),*/ INT16_MIN :                             \
        sox_macro_temp_double - 0.5 :                           \
      sox_macro_temp_double >= INT16_MAX + 0.5 ?           \
        sox_macro_temp_double > INT16_MAX + 1.0 ?          \
          /*++(clips),*/ INT16_MAX :                           \
          INT16_MAX :                                      \
        sox_macro_temp_double + 0.5                             \
  )


typedef struct alsa_conf
{
    char read_name[256];       /// 声卡名字
    char write_name[256];       /// 声卡名字
    int sample_rate;      /// 48000 16000
    int read_channels;       /// 读取通道数据
    int write_channels;      /// 写通道数据
    snd_pcm_format_t format; /// 采样格式

    snd_pcm_t *write_handle;
    snd_pcm_t *read_handle;

    int buffer_frames;

    // 读声卡线程
    pthread_t read_sound_card_th;
    bool read_sound_card_quit;
    // 读声卡
    int read_sound_card_size;
    char *read_sound_card_buf;
    // 写声卡
    int write_sound_card_size;
    char *write_sound_card_buf;
    // 写声卡线程
    std::thread write_sound_card_th;
    bool write_sound_card_quit;

    // 读取声卡的数据放到队列中
    UnlockQueue *read_sound_card_queue;
}t_alsa_conf;


static t_alsa_conf alsa_conf;

/**
 * @brief 打开声卡
 * @param name 声卡名字
 * @param sample_rate  采样率
 * @param channels 采集或者输出通道
 * @param format 采样格式
 * @param handle 声卡handle
 * @param is_write 以读或写的方式打开声卡
 * @return 0 success, other failed
 */
int open_sound_card(char *name, int samplerate, int channels, snd_pcm_format_t format, snd_pcm_t **handle, bool is_write)
{
    int i;
    int err;

    unsigned int rate = 48000;
    snd_pcm_t *_handle;
    snd_pcm_hw_params_t *hw_params;

    snd_pcm_stream_t stream;
    if(is_write == true)
        stream = SND_PCM_STREAM_PLAYBACK;
    else
        stream = SND_PCM_STREAM_CAPTURE;


    if ((err = snd_pcm_open (&_handle, name, stream, 0)) < 0) {
        error("cannot open audio device %s (%s)\n", name, snd_strerror(err));
        return -EB_PARAM_INVALID;
    }

    fprintf(stdout, "audio %s interface opened\n", is_write==true?"write":"read");

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        error("cannot allocate hardware parameter structure (%s)\n", snd_strerror (err));
        return -EB_NOMEM;
    }


    if ((err = snd_pcm_hw_params_any (_handle, hw_params)) < 0) {
        error("cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
        return -EB_PARAM_INVALID;
    }

    if ((err = snd_pcm_hw_params_set_access (_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        error("cannot set access type (%s)\n",  snd_strerror (err));
        return -EB_PARAM_INVALID;
    }

    if ((err = snd_pcm_hw_params_set_format (_handle, hw_params, format)) < 0) {
        error("cannot set sample format (%s)\n", snd_strerror (err));
        return -EB_PARAM_INVALID;
    }

    if ((err = snd_pcm_hw_params_set_rate_near (_handle, hw_params, &rate, 0)) < 0) {
        error("cannot set sample rate (%s)\n", snd_strerror (err));
        return -EB_PARAM_INVALID;
    }

    if ((err = snd_pcm_hw_params_set_channels (_handle, hw_params, channels)) < 0) {
        error("cannot set channel count (%s)\n", snd_strerror (err));
        return -EB_PARAM_INVALID;
    }

    if ((err = snd_pcm_hw_params (_handle, hw_params)) < 0) {
        error("cannot set parameters (%s)\n", snd_strerror (err));
        return -EB_PARAM_INVALID;
    }


    snd_pcm_hw_params_free (hw_params);

    if ((err = snd_pcm_prepare (_handle)) < 0) {
        error("cannot prepare audio interface for use (%s)\n", snd_strerror (err));
        return -EB_PARAM_INVALID;
    }
    *handle = _handle;
    return 0;
}

int close_sound_card(snd_pcm_t **handle)
{
    if(!handle || !(*handle))
    {
        error("null handle!");
        return -1;
    }
    snd_pcm_close (*handle);
    return 0;
}

// 读取声卡线程
void *read_sound_card_proc(void *param)
{
    t_alsa_conf *conf = (t_alsa_conf *)param;
    int err = 0;
    int size = 2*alsa_conf.read_channels*alsa_conf.buffer_frames;

    char *buf = (char *)malloc(1024*2*alsa_conf.write_channels);
    int start = 0;

    PCM_LOCALS;

    printf("AUDIO_FRAME_SIZE = %d\n", AUDIO_FRAME_SIZE);
    // 分配两个通道的呢次
    float *effects_buf = (float *)malloc(AUDIO_FRAME_SIZE * 2 * sizeof(float ));
    float *effects_buf_0 = effects_buf;
    float *effects_buf_1 = effects_buf + AUDIO_FRAME_SIZE;


#if TEST_TIME==1
    void *hd = time_diff_create();
#endif

    while(!conf->read_sound_card_quit) {
        // 读声卡
        if ((err = snd_pcm_readi(conf->read_handle, conf->read_sound_card_buf, conf->buffer_frames)) != conf->buffer_frames) {
            dbg("%d read from audio interface failed (%s)", err, snd_strerror (err));
            if (err == -EPIPE) {
                // 没有及时取走数据
                /* EPIPE means overrun */
                dbg("underrun overrun\n");
                snd_pcm_prepare(conf->read_handle);
            } else if (err < 0) {
                error("error from writei: %s\n", snd_strerror(err));
            }
            // 不进行退出处理
            // exit (1);
        }
#if TEST_TIME==1
        time_diff_start(hd);
#endif


#if AUDIO_READ_CHN == 2 && AUDIO_WRITE_CHN == 2
        int16_t *from0 = (int16_t *)conf->read_sound_card_buf;
        int16_t *to = (int16_t *)conf->write_sound_card_buf;
        for(int i=0; i < AUDIO_FRAME_SIZE; i++) {
            to[2*i + 0] = from0[2*i + 0];
            to[2*i + 1] = from0[2*i + 1];
            //printf("read from %d\n", from0[2*i + 0]);
        }
#elif AUDIO_READ_CHN == 2 && AUDIO_WRITE_CHN == 4
        uint16_t *from0 = (uint16_t *)conf->read_sound_card_buf;
        uint16_t *from1 = (uint16_t *)read_file_buf;
        uint16_t *to = (uint16_t *)conf->write_sound_card_buf;
        for(int i=0; i < AUDIO_FRAME_SIZE; i++) {
            to[4*i + 0] = from1[2*i + 0];
            to[4*i + 1] = from1[2*i + 1];
            to[4*i + 2] = from0[2*i + 0];
            to[4*i + 3] = from0[2*i + 1];
        }
#else
#error "audio read channel and write channel not def"
#endif


#if 1
        for(int i=0; i < AUDIO_FRAME_SIZE; i++) {
            // 转换成浮点数
            effects_buf_0[i] = INT16_TO_FLOAT(from0[2*i + 0]);
            effects_buf_1[i] = INT16_TO_FLOAT(from0[2*i + 1]);
        }

        for(int i=0; i < AUDIO_FRAME_SIZE; i++) {
            // 将数据转换pcm
            to[2*i + 0] = FLOAT_TO_PCM(effects_buf_0[i]);
            to[2*i + 1] = FLOAT_TO_PCM(effects_buf_1[i]);
        }
#endif

        if(start == 0) {
            // 先写一部分数据填充，防止声卡没有足够的数据出现问题
            snd_pcm_writei(conf->write_handle, buf, AUDIO_FRAME_SIZE);
            start = 1;
        }

        // 写声卡
        err = snd_pcm_writei(conf->write_handle, conf->write_sound_card_buf, conf->buffer_frames);
        if (err == -EPIPE) {
            /* EPIPE means underrun */
            dbg("underrun occurred\n");
            snd_pcm_prepare(conf->write_handle);
            snd_pcm_writei(conf->write_handle, buf, AUDIO_FRAME_SIZE);
        } else if (err < 0) {
            error( "error from writei: %s\n", snd_strerror(err));
        }  else if (err != (int)conf->buffer_frames) {
            error("%d write to audio interface failed (%s)\n",
                  err, snd_strerror (err));
            exit (1);
        }



#if TEST_TIME==1
        time_diff_end(hd);
#endif
    }

#if TEST_TIME==1
    time_diff_delete(&hd);
#endif

    free(effects_buf);
    free(buf);
    return nullptr;
}

int init_alsa()
{
    int ret = 0;

    alsa_conf.sample_rate = 48000;
    alsa_conf.read_channels =  AUDIO_READ_CHN;
    alsa_conf.write_channels = AUDIO_WRITE_CHN;
    alsa_conf.buffer_frames = AUDIO_FRAME_SIZE;  // 1024 , 320 ,160  ...
    alsa_conf.format = SND_PCM_FORMAT_S16_LE;
    sprintf(alsa_conf.read_name, "hw:0,0");   // hw:0,0 line_in,  hw:1,0 hdmi_in,  hw:4,0 usb_in
    sprintf(alsa_conf.write_name, "hw:0,0");  // hw:0,0 line_out, hw:3,0 hdmi_out
    alsa_conf.read_handle = nullptr;
    alsa_conf.write_handle = nullptr;
    alsa_conf.read_sound_card_queue = nullptr;

    // 创建队列缓冲区
    alsa_conf.read_sound_card_queue = new UnlockQueue(1024*alsa_conf.read_channels*2*16);
    alsa_conf.read_sound_card_queue->Initialize();

    dbg("yk debug ");
    // 打开声卡读capture
    ret = open_sound_card(alsa_conf.read_name, alsa_conf.sample_rate, alsa_conf.read_channels, alsa_conf.format, &alsa_conf.read_handle, false);
    if(ret != 0) {
        error("error in read open sound card!");
        return ret;
    }

    // 分配读声卡内存
    alsa_conf.read_sound_card_size = alsa_conf.buffer_frames * snd_pcm_format_width(alsa_conf.format) / 8 * alsa_conf.read_channels;
    alsa_conf.read_sound_card_buf =(char*) malloc(alsa_conf.read_sound_card_size);
    // 分配写声卡内存
    alsa_conf.write_sound_card_size = alsa_conf.buffer_frames * snd_pcm_format_width(alsa_conf.format) / 8 * alsa_conf.write_channels;
    alsa_conf.write_sound_card_buf =(char*) malloc(alsa_conf.write_sound_card_size);

    // 打开声卡写
    ret = open_sound_card(alsa_conf.write_name, alsa_conf.sample_rate, alsa_conf.write_channels, alsa_conf.format, &alsa_conf.write_handle, true);
    if(ret != 0) {
        error("error in write open sound card!");
        return ret;
    }

    // 创建线程读声卡，写声卡
    alsa_conf.read_sound_card_quit = false;
#if SET_THREAD_PRIORITY == 1
    struct sched_param param_read_thread;
    pthread_attr_init(&attr_read_thread);
    errno = pthread_attr_setinheritsched(&attr_read_thread, PTHREAD_EXPLICIT_SCHED);
    if(errno != 0)
    {
        perror("setinherit failed\n");
        return -1;
    }

    /* 设置线程的调度策略：SCHED_FIFO：抢占性调度; SCHED_RR：轮寻式调度；SCHED_OTHER：非实时线程调度策略*/
    ret = pthread_attr_setschedpolicy(&attr_read_thread, SCHED_RR);
    if(ret != 0)
    {
        perror("setpolicy failed\n");
        return -1;
    }
    //设置优先级的级别
    param_read_thread.sched_priority = 1;

    //查看抢占性调度策略的最小跟最大静态优先级的值是多少
    printf("min=%d, max=%d\n", sched_get_priority_min(SCHED_FIFO), sched_get_priority_max(SCHED_FIFO));

    /* 设置线程静态优先级 */
    ret = pthread_attr_setschedparam(&attr_read_thread, &param_read_thread);
    if(ret != 0)
    {
        perror("setparam failed\n");
        return -1;
    }

    ret = pthread_create(&alsa_conf.read_sound_card_th, &attr_read_thread, read_sound_card_proc, &alsa_conf);
    if (ret != 0) {
        error("error to create th:%s", strerror(errno));
        exit(1);
    }
#else
        pthread_create(&alsa_conf.read_sound_card_th, nullptr, read_sound_card_proc, &alsa_conf);
#endif


    dbg("yk debug ");
    return 0;
}
int deinit_alsa()
{
    info("debug quit");

    alsa_conf.write_sound_card_quit = true;
    alsa_conf.read_sound_card_quit = true;

    if(alsa_conf.read_sound_card_th){
        pthread_join(alsa_conf.read_sound_card_th, nullptr);
    }

    if(alsa_conf.write_sound_card_th.get_id() != std::thread::id()) {
        alsa_conf.write_sound_card_th.join();
    }

    if(alsa_conf.read_handle) {
        close_sound_card(&alsa_conf.read_handle);
    }

    if(alsa_conf.write_handle) {
        close_sound_card(&alsa_conf.write_handle);
    }

    if(alsa_conf.read_sound_card_buf){
        free(alsa_conf.read_sound_card_buf);
        alsa_conf.read_sound_card_buf = nullptr;
    }


#if SET_THREAD_PRIORITY == 1
    pthread_attr_destroy(&attr_read_thread);
#endif

    info("debug quit");
    return 0;
}
