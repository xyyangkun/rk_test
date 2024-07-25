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
#include "alsa.h"
#include "time_utils.h"
#include "rkav_interface.h"


#define VERSION "0.2"
// -1 close  关闭日志
// 0 error  default，不想输出太多调试信息
// 1 info   输出info信息
// 2 dbg    输出调试信息
static int dbg_flag = 2;
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
static pthread_attr_t attr_read_line_in_thread;
static pthread_attr_t attr_read_hdmi_in_thread;
#endif

// line_in -> line_out 20
// hdmi_in -> line_out 64
// usb_in  -> hdmi_out 64
// line_in -> hdmi_out 64
// hdmi_in -> hdmi_out 64
// usb_in  -> hdmi_out 64


#define QUEUE_CACHE_SIZE 5
#define QUEUE_SIZE (QUEUE_CACHE_SIZE - 2)

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
    char line_in_read_name[256];       /// line_in 声卡名字
    char line_out_write_name[256];     /// line_out声卡名字

    char hdmi_in_read_name[256];       // hdmi_in 声卡名字
    char usb_in_read_name[256];       // usb_in 声卡名字

    char hdmi_out_write_name[256];     /// hdmi_out声卡名字

    int sample_rate;      /// 48000 16000
    int read_channels;       /// 读取通道数据
    int write_channels;      /// 写通道数据
    snd_pcm_format_t format; /// 采样格式

    snd_pcm_t *line_out_handle;
    snd_pcm_t *line_in_handle;

    snd_pcm_t *hdmi_in_handle;
    snd_pcm_status_t *hdmi_in_status; // snd_pcm_status_alloca 不用释放

    snd_pcm_t *usb_in_handle;
    snd_pcm_t *dec_in_handle;
    snd_pcm_t *hdmi_out_handle;
    snd_pcm_t *uac_out_handle;

    int buffer_frames;

    // 读声卡线程
    pthread_t read_sound_card_th;
    bool read_sound_card_quit;
    pthread_t read_hdmi_in_th;

    // 读line_in声卡
    int line_in_read_size;
    char *line_in_read_buf;
    // 读hdmi_in声卡
    int hdmi_in_read_size;
    char *hdmi_in_read_buf;

    // 读usb_in声卡
    int usb_in_read_size;
    char *usb_in_read_buf;

    // 写line_out声卡
    int line_out_write_size;
    char *line_out_write_buf;

    // 写声卡线程
    std::thread write_sound_card_th;
    bool write_sound_card_quit;

    // 读取声卡的数据放到队列中
    UnlockQueue *line_in_read_queue;
    UnlockQueue *hdmi_in_read_queue;

    UnlockQueue *hdmi_in_cache_queue; // hdmi in缓存队列
    UnlockQueue *usb_in_queue;  // usb in缓存队列
}t_alsa_conf;

typedef struct s_audio_buffer
{
    int64_t ts;     // 时间戳
    int buf_size;   // buf 大小
    char buf[];      // buf 指针
}t_audio_buffer;

static t_alsa_conf alsa_conf;

UnlockQueue * get_usb_queue()
{
    return alsa_conf.usb_in_queue;
}

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
int open_sound_card(char *name, int samplerate, int channels, snd_pcm_format_t format, snd_pcm_t **handle, bool is_write, bool is_block = true)
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

    int mode = 0;
    if(is_block == false)
    {
        mode = SND_PCM_NONBLOCK;
    }
    if ((err = snd_pcm_open (&_handle, name, stream, mode)) < 0) {
        error("cannot open audio device %s (%s)\n", name, snd_strerror(err));
        return -1;
    }

    fprintf(stdout, "audio %s interface opened\n", is_write==true?"write":"read");

    if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
        error("cannot allocate hardware parameter structure (%s)\n", snd_strerror (err));
        return -2;
    }


    if ((err = snd_pcm_hw_params_any (_handle, hw_params)) < 0) {
        error("cannot initialize hardware parameter structure (%s)\n", snd_strerror (err));
        return -1;
    }

    if ((err = snd_pcm_hw_params_set_access (_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        error("cannot set access type (%s)\n",  snd_strerror (err));
        return -1;
    }

    if ((err = snd_pcm_hw_params_set_format (_handle, hw_params, format)) < 0) {
        error("cannot set sample format (%s)\n", snd_strerror (err));
        return -1;
    }

    if ((err = snd_pcm_hw_params_set_rate_near (_handle, hw_params, &rate, 0)) < 0) {
        error("cannot set sample rate (%s)\n", snd_strerror (err));
        return -1;
    }

    if ((err = snd_pcm_hw_params_set_channels (_handle, hw_params, channels)) < 0) {
        error("cannot set channel count (%s)\n", snd_strerror (err));
        return -1;
    }

    if ((err = snd_pcm_hw_params (_handle, hw_params)) < 0) {
        error("cannot set parameters (%s)\n", snd_strerror (err));
        return -1;
    }


    snd_pcm_hw_params_free (hw_params);

    if ((err = snd_pcm_prepare (_handle)) < 0) {
        error("cannot prepare audio interface for use (%s)\n", snd_strerror (err));
        return -1;
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
void *read_hdmi_in_sound_card_proc(void *param)
{
    t_alsa_conf *conf = (t_alsa_conf *)param;
    int err = 0;
    int hdmi_in_buf_size = 2*alsa_conf.read_channels*alsa_conf.buffer_frames;

    char *hdmi_in_buf = (char *)malloc(hdmi_in_buf_size);

#if TEST_TIME==1
    const char *hdmi_in_diff = "/tmp/hdmi_in_time_diff";
    void *hd = time_diff_create(hdmi_in_diff);
#endif

    while(!conf->read_sound_card_quit) {
        // 从队列中获取buffer值
        t_audio_buffer *p_audio_buffer;
        int16_t *p = nullptr;

        bool queue_empty = false;
        if(alsa_conf.hdmi_in_cache_queue->GetDataLen() > 0)
        {
            queue_empty = false;
            alsa_conf.hdmi_in_cache_queue->Get(&p_audio_buffer, sizeof(void*));
            p = (int16_t *)p_audio_buffer->buf;
            p_audio_buffer->buf_size = conf->buffer_frames;
        }
        else
        {
            queue_empty = true;
            error("error in hdmi in read queue size:%d %d\n", alsa_conf.hdmi_in_read_queue->GetDataLen(), alsa_conf.hdmi_in_cache_queue->GetDataLen());
            p = (int16_t *)hdmi_in_buf;
        }


        // 读声卡
        if ((err = snd_pcm_readi(conf->hdmi_in_handle, p, conf->buffer_frames)) != conf->buffer_frames) {
            dbg("%d read from audio interface failed (%s)", err, snd_strerror (err));
            if (err == -EPIPE) {
                // 没有及时取走数据
                /* EPIPE means overrun */
                dbg("underrun overrun\n");
                snd_pcm_prepare(conf->hdmi_in_handle);
            } else if (err < 0) {
                error("error from writei: %s\n", snd_strerror(err));
            }
            // 不进行退出处理
            // exit (1);
        }

#if TEST_TIME==1
        time_diff_start(hd);
#endif


        // 把数据放到队列中
        if(queue_empty == false){
            struct timespec crt_tm = {0, 0};
            clock_gettime(CLOCK_MONOTONIC, &crt_tm);
            uint64_t t= crt_tm.tv_sec * 1000000LL + crt_tm.tv_nsec / 1000;
            // 将采集时间放入buffer中
            p_audio_buffer->ts = t;
            // 队列不为空时，才需要写入
            alsa_conf.hdmi_in_read_queue->Put(&p_audio_buffer, sizeof(void*));
        }




#if TEST_TIME==1
        time_diff_end(hd);
#endif
    }

#if TEST_TIME==1
    time_diff_delete(&hd);
#endif

    free(hdmi_in_buf);
    return nullptr;
}

// volume_value = pow(10, db/20)
static float hdmi_in_volume = 1.0;
static float line_in_volume = 1.0;
static float usb_in_volume = 1.0;
static float mp4_in_volume = 1.0;
static float line_out_volume = 1.0;
void new_set_hdmi_in_volume(float value) {hdmi_in_volume = value;}
void new_set_line_in_volume(float value) {line_in_volume = value;}
void new_set_usb_in_volume(float value)  {usb_in_volume = value;}
void new_set_mp4_in_volume(float value)  {mp4_in_volume = value;}
void new_set_line_out_volume(float value){line_out_volume = value;}
static float rms_factor_peak =  1.0/131072.0;
static float rms_factor_rms =  32.0/131072.0;
static t_meter audio_meter;
static float hdmi_in_peak_l = 0;
static float hdmi_in_vu_l =  0;  // rms
static float hdmi_in_peak_r = 0;
static float hdmi_in_vu_r =  0;   // rms

static float usb_in_peak_l = 0;
static float usb_in_vu_l =  0;  // rms
static float usb_in_peak_r = 0;
static float usb_in_vu_r =  0;   // rms

static float line_in_peak_l = 0;
static float line_in_vu_l =  0;  // rms
static float line_in_peak_r = 0;
static float line_in_vu_r =  0;   // rms

static float line_out_peak_l = 0;
static float line_out_vu_l =  0;  // rms
static float line_out_peak_r = 0;
static float line_out_vu_r =  0;   // rms

inline void cacl_vu_pk(float *effects_buf_0, float *effects_buf_1, float& peak_l, float& vu_l, float& peak_r, float& vu_r )
{
    float tmp, x;

    // 计算vu pk
    for(int i=0; i < AUDIO_FRAME_SIZE; i++) {
        // 左声道 peak
        x = fabs(effects_buf_0[i]);
        tmp = peak_l + (x - peak_l)* rms_factor_peak;
        peak_l = tmp > x ? tmp:x;
        // 左声道 vu
        vu_l = vu_l + (x - vu_l) * rms_factor_rms;
    }
    for(int i=0; i < AUDIO_FRAME_SIZE; i++) {
        // 右声道 peak
        x = fabs(effects_buf_1[i]);
        tmp = peak_r + (x - peak_r)* rms_factor_peak;
        peak_r = tmp > x ? tmp:x;
        // 右声道 vu
        vu_r = vu_r + (x - vu_r) * rms_factor_rms;
    }
}

// 写声卡线程
void new_get_meter(void *_meter)
{

    int32_t i_peak_l = 0;
    int32_t i_vu_l =  0;
    int32_t i_peak_r = 0;
    int32_t i_vu_r =  0;

    // hdmi_in
    {
        i_peak_l = (int32_t)(20.0*log10(  hdmi_in_peak_l));
        i_vu_l   = (int32_t)(20.0 * log10(hdmi_in_vu_l));
        i_peak_r = (int32_t)(20.0*log10(  hdmi_in_peak_r));
        i_vu_r   = (int32_t)(20.0 * log10(hdmi_in_vu_r));
        //i_peak_l += 6;
        i_vu_l   += 6;
        //i_peak_r += 6;
        i_vu_r   += 6;
        // 检测vu pk值过大，并输出
        /*
        if(i_peak_l > 0 || i_vu_l > 0|| i_peak_r > 0|| i_vu_r > 0){
            osee_error("error in vu_pk peak_l=%d peak_r = %d rms_l=%d rms_r=%d",
                       i_peak_l, i_peak_r, i_vu_l, i_vu_r );
        }
         //*/
        //osee_print("in vu_pk peak_l=%f==%d peak_r = %f==%d rms_l=%f==%d rms_r=%f==%d",
        //           peak_l, i_peak_l, peak_r, i_peak_r, vu_l, i_vu_l, vu_r, i_vu_r );
        i_peak_l= i_peak_l < -40 ? -40 : i_peak_l > 0 ? 0 : i_peak_l;
        i_vu_l  = i_vu_l   < -40 ? -40 : i_vu_l   > 0 ? 0 : i_vu_l  ;
        i_peak_r= i_peak_r < -40 ? -40 : i_peak_r > 0 ? 0 : i_peak_r;
        i_vu_r  = i_vu_r   < -40 ? -40 : i_vu_r   > 0 ? 0 : i_vu_r  ;

        audio_meter.hdmi_in.pk_left  = i_peak_l;
        audio_meter.hdmi_in.vu_left  = i_vu_l  ;
        audio_meter.hdmi_in.pk_right = i_peak_r;
        audio_meter.hdmi_in.vu_right = i_vu_r  ;
    }

    // line_in
    {
        i_peak_l = (int32_t)(20.0*log10(  line_in_peak_l));
        i_vu_l   = (int32_t)(20.0 * log10(line_in_vu_l));
        i_peak_r = (int32_t)(20.0*log10(  line_in_peak_r));
        i_vu_r   = (int32_t)(20.0 * log10(line_in_vu_r));
        //i_peak_l += 6;
        i_vu_l   += 6;
        //i_peak_r += 6;
        i_vu_r   += 6;
        // 检测vu pk值过大，并输出
        /*
        if(i_peak_l > 0 || i_vu_l > 0|| i_peak_r > 0|| i_vu_r > 0){
            osee_error("error in vu_pk peak_l=%d peak_r = %d rms_l=%d rms_r=%d",
                       i_peak_l, i_peak_r, i_vu_l, i_vu_r );
        }
         //*/
        //osee_print("in vu_pk peak_l=%f==%d peak_r = %f==%d rms_l=%f==%d rms_r=%f==%d",
        //           peak_l, i_peak_l, peak_r, i_peak_r, vu_l, i_vu_l, vu_r, i_vu_r );
        i_peak_l= i_peak_l < -40 ? -40 : i_peak_l > 0 ? 0 : i_peak_l;
        i_vu_l  = i_vu_l   < -40 ? -40 : i_vu_l   > 0 ? 0 : i_vu_l  ;
        i_peak_r= i_peak_r < -40 ? -40 : i_peak_r > 0 ? 0 : i_peak_r;
        i_vu_r  = i_vu_r   < -40 ? -40 : i_vu_r   > 0 ? 0 : i_vu_r  ;

        audio_meter.line_mic.pk_left  = i_peak_l;
        audio_meter.line_mic.vu_left  = i_vu_l  ;
        audio_meter.line_mic.pk_right = i_peak_r;
        audio_meter.line_mic.vu_right = i_vu_r  ;
    }

    // usb_in
    {
        i_peak_l = (int32_t)(20.0*log10(  usb_in_peak_l));
        i_vu_l   = (int32_t)(20.0 * log10(usb_in_vu_l));
        i_peak_r = (int32_t)(20.0*log10(  usb_in_peak_r));
        i_vu_r   = (int32_t)(20.0 * log10(usb_in_vu_r));
        //i_peak_l += 6;
        i_vu_l   += 6;
        //i_peak_r += 6;
        i_vu_r   += 6;
        // 检测vu pk值过大，并输出
        /*
        if(i_peak_l > 0 || i_vu_l > 0|| i_peak_r > 0|| i_vu_r > 0){
            osee_error("error in vu_pk peak_l=%d peak_r = %d rms_l=%d rms_r=%d",
                       i_peak_l, i_peak_r, i_vu_l, i_vu_r );
        }
         //*/
        //osee_print("in vu_pk peak_l=%f==%d peak_r = %f==%d rms_l=%f==%d rms_r=%f==%d",
        //           peak_l, i_peak_l, peak_r, i_peak_r, vu_l, i_vu_l, vu_r, i_vu_r );
        i_peak_l= i_peak_l < -40 ? -40 : i_peak_l > 0 ? 0 : i_peak_l;
        i_vu_l  = i_vu_l   < -40 ? -40 : i_vu_l   > 0 ? 0 : i_vu_l  ;
        i_peak_r= i_peak_r < -40 ? -40 : i_peak_r > 0 ? 0 : i_peak_r;
        i_vu_r  = i_vu_r   < -40 ? -40 : i_vu_r   > 0 ? 0 : i_vu_r  ;

        audio_meter.usb_in.pk_left  = i_peak_l;
        audio_meter.usb_in.vu_left  = i_vu_l  ;
        audio_meter.usb_in.pk_right = i_peak_r;
        audio_meter.usb_in.vu_right = i_vu_r  ;
    }


    // line_out
    {
        i_peak_l = (int32_t)(20.0*log10(  line_out_peak_l));
        i_vu_l   = (int32_t)(20.0 * log10(line_out_vu_l));
        i_peak_r = (int32_t)(20.0*log10(  line_out_peak_r));
        i_vu_r   = (int32_t)(20.0 * log10(line_out_vu_r));
        //i_peak_l += 6;
        i_vu_l   += 6;
        //i_peak_r += 6;
        i_vu_r   += 6;
        // 检测vu pk值过大，并输出
        /*
        if(i_peak_l > 0 || i_vu_l > 0|| i_peak_r > 0|| i_vu_r > 0){
            osee_error("error in vu_pk peak_l=%d peak_r = %d rms_l=%d rms_r=%d",
                       i_peak_l, i_peak_r, i_vu_l, i_vu_r );
        }
         //*/
        //osee_print("in vu_pk peak_l=%f==%d peak_r = %f==%d rms_l=%f==%d rms_r=%f==%d",
        //           peak_l, i_peak_l, peak_r, i_peak_r, vu_l, i_vu_l, vu_r, i_vu_r );
        i_peak_l= i_peak_l < -40 ? -40 : i_peak_l > 0 ? 0 : i_peak_l;
        i_vu_l  = i_vu_l   < -40 ? -40 : i_vu_l   > 0 ? 0 : i_vu_l  ;
        i_peak_r= i_peak_r < -40 ? -40 : i_peak_r > 0 ? 0 : i_peak_r;
        i_vu_r  = i_vu_r   < -40 ? -40 : i_vu_r   > 0 ? 0 : i_vu_r  ;

        audio_meter.out.pk_left  = i_peak_l;
        audio_meter.out.vu_left  = i_vu_l  ;
        audio_meter.out.pk_right = i_peak_r;
        audio_meter.out.vu_right = i_vu_r  ;
    }
    memcpy(_meter, &audio_meter, sizeof(audio_meter));
}

// 读取声卡线程
void *read_line_in_sound_card_proc(void *param)
{
    t_alsa_conf *conf = (t_alsa_conf *)param;
    int err = 0;
    int queue_read_size = sizeof(int16_t) * alsa_conf.read_channels * alsa_conf.buffer_frames;

    char *buf = (char *)malloc(alsa_conf.buffer_frames*20*alsa_conf.write_channels);

    char *hdmi_in_read_buf = (char *)malloc(alsa_conf.buffer_frames*20*alsa_conf.write_channels);


    char *usb_in_read_buf = (char *)malloc(alsa_conf.buffer_frames*20*alsa_conf.write_channels);


    int tmp[1024] = {0};

    int start = 0;

    int hdmi_start = 0;

    PCM_LOCALS;

    printf("AUDIO_FRAME_SIZE = %d\n", AUDIO_FRAME_SIZE);

#define POINT_SIZE ( 2 * sizeof(int16_t))

    // 分配 hdmi_in 通道数据
    float *hdmi_in_effects_buf = (float *)malloc(AUDIO_FRAME_SIZE * 2 * sizeof(float ));
    float *hdmi_in_effects_buf_0 = hdmi_in_effects_buf;
    float *hdmi_in_effects_buf_1 = hdmi_in_effects_buf + AUDIO_FRAME_SIZE;

    // 分配 usb_in 通道数据
    float *usb_in_effects_buf = (float *)malloc(AUDIO_FRAME_SIZE * 2 * sizeof(float ));
    float *usb_in_effects_buf_0 = usb_in_effects_buf;
    float *usb_in_effects_buf_1 = usb_in_effects_buf + AUDIO_FRAME_SIZE;

    // 分配 usb_in 通道数据
    float *mp4_in_effects_buf = (float *)malloc(AUDIO_FRAME_SIZE * 2 * sizeof(float ));
    float *mp4_in_effects_buf_0 = mp4_in_effects_buf;
    float *mp4_in_effects_buf_1 = mp4_in_effects_buf + AUDIO_FRAME_SIZE;

    // 分配 line_in 通道数据
    float *line_in_effects_buf = (float *)malloc(AUDIO_FRAME_SIZE * 2 * sizeof(float ));
    float *line_in_effects_buf_0 = line_in_effects_buf;
    float *line_in_effects_buf_1 = line_in_effects_buf + AUDIO_FRAME_SIZE;

    // 分配 line_out 通道数据
    float *line_out_effects_buf = (float *)malloc(AUDIO_FRAME_SIZE * 2 * sizeof(float ));
    float *line_out_effects_buf_0 = line_out_effects_buf;
    float *line_out_effects_buf_1 = line_out_effects_buf + AUDIO_FRAME_SIZE;

#if TEST_TIME==1
    const char *line_in_out = "/tmp/audio_line_time_diff";
    void *hd = time_diff_create(line_in_out);
#endif

    char *hdmi_in_local_queue_buf = (char *)malloc(queue_read_size);
    UnlockQueue *hdmi_in_local_queue = nullptr;
    int hdmi_in_local_queue_size = AUDIO_FRAME_SIZE*40;
    hdmi_in_local_queue = new UnlockQueue( hdmi_in_local_queue_size * 2 * sizeof(int16_t) );
    hdmi_in_local_queue->Initialize();
    bool hdmi_in_start_read = false;

    bool usb_in_start_read = false;

    while(!conf->read_sound_card_quit) {

        static int count = 0;

        {


            {

              // 读line_in声卡
              if ((err = snd_pcm_readi(conf->line_in_handle, conf->line_in_read_buf, conf->buffer_frames)) != conf->buffer_frames) {
                  //printf("%d read from audio interface failed (%s)", err, snd_strerror (err));
                  if (err == -EPIPE) {
                      // 没有及时取走数据
                      /* EPIPE means overrun */
                      dbg("underrun overrun\n");
                      snd_pcm_prepare(conf->line_in_handle);
                  } else if (err < 0) {
                      error("error from writei: %s\n", snd_strerror(err));
                  }
                  // 不进行退出处理
                  // exit (1);
              }

            }

            {
#if 0

                 snd_pcm_status_alloca(&alsa_conf.hdmi_in_status);
                 // 获取状态
                 int err = snd_pcm_status(conf->hdmi_in_handle, conf->hdmi_in_status);
                 if (err < 0) {
                     printf("无法获取 PCM 状态: %s\n", snd_strerror(err));
                 }
                 snd_pcm_state_t state = snd_pcm_status_get_state(conf->hdmi_in_status);
                 snd_pcm_uframes_t avail = snd_pcm_status_get_avail(conf->hdmi_in_status);
                 snd_pcm_uframes_t delay = snd_pcm_status_get_delay(conf->hdmi_in_status);
                 printf("-----------------delay:%d avail:%d state=%d \n", delay, avail, state);
                 if (snd_pcm_status_get_state(conf->hdmi_in_status) == SND_PCM_STATE_XRUN) {
                     printf("--- xrun\n");
                 }
                 printf("################ mod=%d\n", mod);
#endif

                 //for(int i=0; i<mod; i++)
                 while(1)
                 {
#if 1
                     if(!alsa_conf.hdmi_in_status)
                        snd_pcm_status_alloca(&alsa_conf.hdmi_in_status);
                     // 获取状态
                     int err = snd_pcm_status(conf->hdmi_in_handle, conf->hdmi_in_status);
                     if (err < 0) {
                         printf("无法获取 PCM 状态: %s\n", snd_strerror(err));
                     }
                     snd_pcm_state_t state = snd_pcm_status_get_state(conf->hdmi_in_status);
                     snd_pcm_uframes_t avail = snd_pcm_status_get_avail(conf->hdmi_in_status);
                     snd_pcm_uframes_t delay = snd_pcm_status_get_delay(conf->hdmi_in_status);
                     //snd_pcm_status_free(conf->hdmi_in_status);
                     // printf(">>>>>>>>>>>>>>>>> delay:%d avail:%d state=%d \n", delay, avail, state);
                     if(state == SND_PCM_STATE_RUNNING && avail < AUDIO_FRAME_SIZE)
                     {
                         // printf("no enough data! state=%d %d\n", state, avail);
                         break;
                     }
#endif
                     // 读声卡 i次
                     err = snd_pcm_readi(conf->hdmi_in_handle, hdmi_in_read_buf, conf->buffer_frames);
                     // printf("err=%d %d\n", err, conf->buffer_frames);
                     if (err != conf->buffer_frames) {
                         printf("%d read from audio interface failed (%s)", err, snd_strerror (err));
                         if (err == -EPIPE) {
                             // 没有及时取走数据
                             /* EPIPE means overrun */
                             dbg("underrun overrun\n");
                             snd_pcm_prepare(conf->hdmi_in_handle);
                         } else if (err < 0) {
                             error("error from readi: %s\n", snd_strerror(err));
                         }
                         // 不进行退出处理
                         break;
                     }else{
                         // printf(">>>>>>>>>>>>>>>>> delay:%d avail:%d state=%d \n", delay, avail, state);
                         int queue_size = hdmi_in_local_queue->GetDataLen();
                         if(queue_size > AUDIO_FRAME_SIZE * 2 * sizeof(int16_t) * 35)
                         {
                             printf("too much data! will release! queue size:%d\n", queue_size);
                             info("too much data! will release! queue size:%d\n", queue_size);
                         } else
                         {
                             hdmi_in_local_queue->Put(hdmi_in_read_buf, conf->buffer_frames*2*2);
                         }

                    }

                }
            }
        }



#if TEST_TIME==1
        time_diff_start(hd);
#endif


        int queue_size = hdmi_in_local_queue->GetDataLen();
        // printf("hdmi_in_local_queue size:%d\n", queue_size / POINT_SIZE);

        /*
         *  延迟    : 50    40    30    20   10   1
         *  缓存点数 : 2400  1920  1440  960  480  0
         */
        int hdmi_in_start_size =  3 * AUDIO_FRAME_SIZE * POINT_SIZE; // 2包  - 3包之间 [960, 1920)
        // 队列中有足够的数据时queue_size >= hdmi_in_start_size， 才开始取数据
        if(queue_size >= hdmi_in_start_size && hdmi_in_start_read == false)
        {
            hdmi_in_start_read = true;
            memset(hdmi_in_read_buf, 0, conf->buffer_frames * POINT_SIZE);
            // printf("hdmi_in will start read! queue_size:%d\n", queue_size / POINT_SIZE);
        }
        else if(queue_size > 0 && hdmi_in_start_read == true)
        {
            // if(count%10 == 0)printf("queue_size = %d\n", queue_size);
            if(queue_size <  2 * AUDIO_FRAME_SIZE * POINT_SIZE )
            {
                // 数据过少，插点
                // 少取一个点，最后一个点进行复制
                hdmi_in_local_queue->Get(hdmi_in_read_buf, (conf->buffer_frames - 1) * POINT_SIZE);
                int32_t *p = (int32_t *)hdmi_in_read_buf;
                p[conf->buffer_frames - 1] = p[conf->buffer_frames - 2];
                // printf("hdmi too few data ! will insert data queue_size:%d\n", queue_size / POINT_SIZE);
            }
            else if(queue_size >=  4 * AUDIO_FRAME_SIZE * POINT_SIZE)
            {
                // 数据过多，丢点
                // 多取一个点，最后一个点丢弃
                hdmi_in_local_queue->Get(hdmi_in_read_buf, (conf->buffer_frames + 1) * POINT_SIZE);
                // printf("hdmi too much data ! will loss data queue_size:%d\n", queue_size / POINT_SIZE);
            }
            else
            {
                hdmi_in_local_queue->Get(hdmi_in_read_buf, conf->buffer_frames* POINT_SIZE);
            }

        }
        else{
            // printf("--------------> error!!! no enough buf!!\n");
            //continue;
            hdmi_in_start_read = false;
            memset(hdmi_in_read_buf, 0, conf->buffer_frames*2*2);
        }

//



        /*  usb      7     6     5     4     3     2    1    0
         *  延迟    : 70    60    50    40    30    20   10   0
         *  缓存点数 : 3360  2880  2400  1920  1440  960  480  0
         */
        int usb_in_start_size =  4 * AUDIO_FRAME_SIZE * POINT_SIZE; // 4包  - 7包之间 [1440, 3360)

        int usb_queue_size = alsa_conf.usb_in_queue->GetDataLen();
        //printf("usb queue size:%d\n", usb_queue_size/(POINT_SIZE));

        // usb队列中有足够的数据时 usb_queue_size >= usb_in_start_size， 才开始取数据
        if(usb_queue_size >= usb_in_start_size && usb_in_start_read == false)
        {
            usb_in_start_read = true;
            memset(usb_in_read_buf, 0, conf->buffer_frames * POINT_SIZE);
            // printf("usb_in will start read! queue_size:%d\n", queue_size / POINT_SIZE);
        }
        else if(usb_queue_size > 0 && usb_in_start_read == true)
        {
            if(usb_queue_size <  3 * AUDIO_FRAME_SIZE * POINT_SIZE )
            {
                // 数据过少，插点
                // 少取一个点，最后一个点进行复制
                alsa_conf.usb_in_queue->Get(usb_in_read_buf, (conf->buffer_frames - 1) * POINT_SIZE);
                int32_t *p = (int32_t *)usb_in_read_buf;
                p[conf->buffer_frames - 1] = p[conf->buffer_frames - 2];
                // printf("usb too few data ! will insert data queue_size:%d\n", usb_queue_size / POINT_SIZE);
            }
            else if(usb_queue_size >=  7 * AUDIO_FRAME_SIZE * POINT_SIZE)
            {
                // 数据过多，丢点
                // 多取一个点，最后一个点丢弃
                alsa_conf.usb_in_queue->Get(usb_in_read_buf, (conf->buffer_frames + 1) * POINT_SIZE);
                // printf("usb too much data ! will loss data queue_size:%d\n", usb_queue_size / POINT_SIZE);
            }
            else
            {
                alsa_conf.usb_in_queue->Get(usb_in_read_buf, conf->buffer_frames* POINT_SIZE);
            }

        }
        else{
            // printf("--------------> error!!! no enough buf!!\n");
            //continue;
            usb_in_start_read = false;
            memset(usb_in_read_buf, 0, conf->buffer_frames*2*2);
        }


        int16_t *from_hdmi_in = (int16_t *)hdmi_in_read_buf;
        int16_t *from_usb_in = (int16_t *)usb_in_read_buf;

        int16_t *from_line_in = (int16_t *)conf->line_in_read_buf;
        int16_t *to_out = (int16_t *)conf->line_out_write_buf;

        // 转换 并计算
        for(int i=0; i < AUDIO_FRAME_SIZE; i++) {
            // 转换成浮点数
            hdmi_in_effects_buf_0[i] = INT16_TO_FLOAT(from_hdmi_in[2 * i + 0]);
            hdmi_in_effects_buf_1[i] = INT16_TO_FLOAT(from_hdmi_in[2 * i + 1]);
        }
        cacl_vu_pk(hdmi_in_effects_buf_0, hdmi_in_effects_buf_1,
                   hdmi_in_peak_l, hdmi_in_vu_l,
                   hdmi_in_peak_r, hdmi_in_vu_r);

        for(int i=0; i < AUDIO_FRAME_SIZE; i++) {
            // 转换成浮点数
            line_in_effects_buf_0[i] = INT16_TO_FLOAT(from_line_in[2 * i + 0]);
            line_in_effects_buf_1[i] = INT16_TO_FLOAT(from_line_in[2 * i + 1]);
        }
        cacl_vu_pk(line_in_effects_buf_0, line_in_effects_buf_1,
                   line_in_peak_l, line_in_vu_l,
                   line_in_peak_r, line_in_vu_r);
        for(int i=0; i < AUDIO_FRAME_SIZE; i++) {
            // 转换成浮点数
            usb_in_effects_buf_0[i] = INT16_TO_FLOAT(from_usb_in[2 * i + 0]);
            usb_in_effects_buf_1[i] = INT16_TO_FLOAT(from_usb_in[2 * i + 1]);
        }
        cacl_vu_pk(usb_in_effects_buf_0, usb_in_effects_buf_1,
                   usb_in_peak_l, usb_in_vu_l,
                   usb_in_peak_r, usb_in_vu_r);


        // 混音
        for(int i=0; i < AUDIO_FRAME_SIZE; i++) {
            // 转换成浮点数
            line_out_effects_buf_0[i] = hdmi_in_effects_buf_0[i] * hdmi_in_volume + line_in_effects_buf_0[i] * line_in_volume + usb_in_effects_buf_0[i] * usb_in_volume;
            line_out_effects_buf_1[i] = hdmi_in_effects_buf_1[i] * hdmi_in_volume + line_in_effects_buf_1[i] * line_in_volume + usb_in_effects_buf_1[i] * usb_in_volume;
        }

        cacl_vu_pk(line_out_effects_buf_0, line_out_effects_buf_1,
                   line_out_peak_l, line_out_vu_l,
                   line_out_peak_r, line_out_vu_r);
        // 混音后 还原
        for(int i=0; i < AUDIO_FRAME_SIZE; i++) {
            // 将数据转换pcm
            to_out[2 * i + 0] = FLOAT_TO_PCM(line_out_effects_buf_0[i] * line_out_volume);
            to_out[2 * i + 1] = FLOAT_TO_PCM(line_out_effects_buf_1[i] * line_out_volume);
        }


        if(start == 0) {
            // 先写一部分数据填充，防止声卡没有足够的数据出现问题
            snd_pcm_writei(conf->line_out_handle, buf, AUDIO_FRAME_SIZE);
            start = 1;
        }


        // 写声卡
        err = snd_pcm_writei(conf->line_out_handle, conf->line_out_write_buf, conf->buffer_frames);
        if (err == -EPIPE) {
            /* EPIPE means underrun */
            dbg("line out underrun occurred\n");
            snd_pcm_prepare(conf->line_out_handle);
            snd_pcm_writei(conf->line_out_handle, buf, AUDIO_FRAME_SIZE);
        } else if (err < 0) {
            error( "error from line out writei: %s\n", snd_strerror(err));
        }  else if (err != (int)conf->buffer_frames) {
            error("%d write to audio interface failed (%s)\n",
                  err, snd_strerror (err));
            //exit (1);
        }

#if 1
        if(hdmi_start == 0) {
            // 先写一部分数据填充，防止声卡没有足够的数据出现问题
            snd_pcm_writei(conf->hdmi_out_handle, buf, AUDIO_FRAME_SIZE);
            hdmi_start = 1;
        }

        // 写hdmi_out声卡
        err = snd_pcm_writei(conf->hdmi_out_handle, conf->line_out_write_buf, conf->buffer_frames);
        if (err == -EPIPE) {
            /* EPIPE means underrun */
            dbg("hdmi out underrun occurred\n");
            snd_pcm_prepare(conf->hdmi_out_handle);
            snd_pcm_writei(conf->hdmi_out_handle, buf, AUDIO_FRAME_SIZE);
        } else if (err < 0) {
            error( "error from hdmi out  writei: %s\n", snd_strerror(err));
        }  else if (err != (int)conf->buffer_frames) {
            error("%d write to hdmi out audio interface failed (%s)\n",
                  err, snd_strerror (err));
            //exit (1);
        }

#endif


#if TEST_TIME==1
        time_diff_end(hd);
#endif
    }

#if TEST_TIME==1
    time_diff_delete(&hd);
#endif

    free(hdmi_in_effects_buf);
    free(usb_in_effects_buf);
    free(mp4_in_effects_buf);
    free(line_in_effects_buf);
    free(line_out_effects_buf);

    free(buf);
    free(hdmi_in_read_buf);
    free(hdmi_in_local_queue_buf);
    free(usb_in_read_buf);
    if(hdmi_in_local_queue) {
        delete hdmi_in_local_queue;
        hdmi_in_local_queue = nullptr;
    }
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
    sprintf(alsa_conf.line_in_read_name, "hw:0,0");   // hw:0,0 line_in,  hw:1,0 hdmi_in,  hw:4,0 usb_in
    sprintf(alsa_conf.hdmi_in_read_name, "hw:1,0");   // hw:1,0 hdmi_in   固定
    sprintf(alsa_conf.usb_in_read_name, "hw:4,0");    // hw:4,0 usb_in   固定

    sprintf(alsa_conf.line_out_write_name, "hw:0,0");  // hw:0,0 line_out

    sprintf(alsa_conf.hdmi_out_write_name, "hw:3,0");  // hw:3,0 hdmi_out  固定

    alsa_conf.line_in_handle = nullptr;
    alsa_conf.hdmi_in_handle = nullptr;
    alsa_conf.hdmi_in_status = nullptr;

    alsa_conf.usb_in_handle = nullptr;
    alsa_conf.dec_in_handle = nullptr;
    alsa_conf.uac_out_handle = nullptr;
    alsa_conf.hdmi_out_handle = nullptr;

    alsa_conf.line_out_handle = nullptr;
    alsa_conf.line_in_read_queue = nullptr;
    alsa_conf.hdmi_in_read_queue = nullptr;
    alsa_conf.hdmi_in_cache_queue = nullptr;
    alsa_conf.usb_in_queue  = nullptr;

    // 创建队列缓冲区
    alsa_conf.line_in_read_queue = new UnlockQueue(1024 * alsa_conf.read_channels * 2 * 16);
    alsa_conf.line_in_read_queue->Initialize();

    alsa_conf.hdmi_in_read_queue = new UnlockQueue(/*QUEUE_SIZE*/8 * sizeof(void*));
    alsa_conf.hdmi_in_read_queue->Initialize();

    // 分配内存
    {
        alsa_conf.hdmi_in_cache_queue = new UnlockQueue(/*QUEUE_CACHE_SIZE*/ 8 * sizeof(void*));
        alsa_conf.hdmi_in_cache_queue->Initialize();
        for(int i=0; i<8; i++)
        {
            // 要多分配内存
            t_audio_buffer *p = (t_audio_buffer *)malloc( AUDIO_FRAME_SIZE * alsa_conf.read_channels * 2*2);
            assert(p);
            alsa_conf.hdmi_in_cache_queue->Put((const unsigned char *)&p, sizeof(void*));
        }
        info("===============> hdmi in cache size:%d %d\n", alsa_conf.hdmi_in_cache_queue->GetDataLen(), alsa_conf.hdmi_in_cache_queue->IsFull());

        alsa_conf.usb_in_queue = new UnlockQueue(AUDIO_FRAME_SIZE * 20 * 4);
        alsa_conf.usb_in_queue->Initialize();
    }


    dbg("yk debug ");
    // 打开line_in声卡读capture
    ret = open_sound_card(alsa_conf.line_in_read_name, alsa_conf.sample_rate, alsa_conf.read_channels, alsa_conf.format, &alsa_conf.line_in_handle, false);
    if(ret != 0) {
        error("error in read open sound card!");
        return ret;
    }
    // 打开hdmi_in声卡读capture
    ret = open_sound_card(alsa_conf.hdmi_in_read_name, alsa_conf.sample_rate, alsa_conf.read_channels, alsa_conf.format, &alsa_conf.hdmi_in_handle, false, false);
    if(ret != 0) {
        error("error in read open sound card!");
        return ret;
    }


    // // 打开usb_in声卡读capture
    // ret = open_sound_card(alsa_conf.usb_in_read_name, alsa_conf.sample_rate, alsa_conf.read_channels, alsa_conf.format, &alsa_conf.usb_in_handle, false);
    // if(ret != 0) {
    //     error("error in read open sound card!");
    //     return ret;
    // }

    // 分配读line_in声卡内存
    alsa_conf.line_in_read_size = alsa_conf.buffer_frames * snd_pcm_format_width(alsa_conf.format) / 8 * alsa_conf.read_channels;
    alsa_conf.line_in_read_buf =(char*) malloc(alsa_conf.line_in_read_size);

    // 分配读hdmi_in声卡内存
    alsa_conf.hdmi_in_read_size = alsa_conf.buffer_frames * snd_pcm_format_width(alsa_conf.format) / 8 * alsa_conf.read_channels;
    alsa_conf.hdmi_in_read_buf =(char*) malloc(alsa_conf.line_in_read_size);

    // 分配读usb_in声卡内存
    alsa_conf.usb_in_read_size = alsa_conf.buffer_frames * snd_pcm_format_width(alsa_conf.format) / 8 * alsa_conf.read_channels;
    alsa_conf.usb_in_read_buf =(char*) malloc(alsa_conf.line_in_read_size);


    // 分配写声卡内存
    alsa_conf.line_out_write_size = alsa_conf.buffer_frames * snd_pcm_format_width(alsa_conf.format) / 8 * alsa_conf.write_channels;
    alsa_conf.line_out_write_buf =(char*) malloc(alsa_conf.line_out_write_size);

    // 打开line_out声卡写
    ret = open_sound_card(alsa_conf.line_out_write_name, alsa_conf.sample_rate, alsa_conf.write_channels, alsa_conf.format, &alsa_conf.line_out_handle, true);
    if(ret != 0) {
        error("error in write open sound card!");
        return ret;
    }


    // 打开hdmi_out声卡写
    ret = open_sound_card(alsa_conf.hdmi_out_write_name, alsa_conf.sample_rate, alsa_conf.write_channels, alsa_conf.format, &alsa_conf.hdmi_out_handle, true);
    if(ret != 0) {
        error("error in write open sound card!");
        return ret;
    }


    // 创建线程读声卡，写声卡
    alsa_conf.read_sound_card_quit = false;
#if SET_THREAD_PRIORITY == 1
    struct sched_param param_read_thread;
    {
        pthread_attr_init(&attr_read_line_in_thread);
        errno = pthread_attr_setinheritsched(&attr_read_line_in_thread, PTHREAD_EXPLICIT_SCHED);
        if(errno != 0)
        {
            perror("setinherit failed\n");
            return -1;
        }

        /* 设置线程的调度策略：SCHED_FIFO：抢占性调度; SCHED_RR：轮寻式调度；SCHED_OTHER：非实时线程调度策略*/
        ret = pthread_attr_setschedpolicy(&attr_read_line_in_thread, SCHED_RR);
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
        ret = pthread_attr_setschedparam(&attr_read_line_in_thread, &param_read_thread);
        if(ret != 0)
        {
            perror("setparam failed\n");
            return -1;
        }
    }
    {
        pthread_attr_init(&attr_read_hdmi_in_thread);
        errno = pthread_attr_setinheritsched(&attr_read_hdmi_in_thread, PTHREAD_EXPLICIT_SCHED);
        if(errno != 0)
        {
            perror("setinherit failed\n");
            return -1;
        }

        /* 设置线程的调度策略：SCHED_FIFO：抢占性调度; SCHED_RR：轮寻式调度；SCHED_OTHER：非实时线程调度策略*/
        ret = pthread_attr_setschedpolicy(&attr_read_hdmi_in_thread, SCHED_RR);
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
        ret = pthread_attr_setschedparam(&attr_read_hdmi_in_thread, &param_read_thread);
        if(ret != 0)
        {
            perror("setparam failed\n");
            return -1;
        }
    }

    ret = pthread_create(&alsa_conf.read_sound_card_th, &attr_read_line_in_thread, read_line_in_sound_card_proc, &alsa_conf);
    if (ret != 0) {
        error("error to create th:%s", strerror(errno));
        exit(1);
    }


#if 0
    ret = pthread_create(&alsa_conf.read_hdmi_in_th, &attr_read_hdmi_in_thread, read_hdmi_in_sound_card_proc, &alsa_conf);
    if (ret != 0) {
        error("error to create th:%s", strerror(errno));
        exit(1);
    }
#endif

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
    if(alsa_conf.read_hdmi_in_th){
        pthread_join(alsa_conf.read_hdmi_in_th, nullptr);
    }

    if(alsa_conf.write_sound_card_th.get_id() != std::thread::id()) {
        alsa_conf.write_sound_card_th.join();
    }

    if(alsa_conf.line_in_handle) {
        close_sound_card(&alsa_conf.line_in_handle);
        alsa_conf.line_in_handle = nullptr;
    }
    if(alsa_conf.hdmi_in_handle) {
        close_sound_card(&alsa_conf.hdmi_in_handle);
        alsa_conf.hdmi_in_handle = nullptr;
    }
    if(alsa_conf.dec_in_handle) {
        close_sound_card(&alsa_conf.dec_in_handle);
        alsa_conf.dec_in_handle = nullptr;
    }
    if(alsa_conf.usb_in_handle) {
        close_sound_card(&alsa_conf.usb_in_handle);
        alsa_conf.usb_in_handle = nullptr;
    }

    if(alsa_conf.line_out_handle) {
        close_sound_card(&alsa_conf.line_out_handle);
        alsa_conf.line_out_handle = nullptr;
    }

    if(alsa_conf.hdmi_out_handle) {
        close_sound_card(&alsa_conf.hdmi_out_handle);
        alsa_conf.hdmi_out_handle = nullptr;
    }

    if(alsa_conf.uac_out_handle) {
        close_sound_card(&alsa_conf.uac_out_handle);
        alsa_conf.uac_out_handle = nullptr;
    }


    if(alsa_conf.line_in_read_buf){
        free(alsa_conf.line_in_read_buf);
        alsa_conf.line_in_read_buf = nullptr;
    }

    if(alsa_conf.line_in_read_queue) {
        delete(alsa_conf.line_in_read_queue);
        alsa_conf.line_in_read_queue = nullptr;
    }

    if(alsa_conf.hdmi_in_cache_queue) {
        while(alsa_conf.hdmi_in_cache_queue->GetDataLen() > 0)
        {
            t_audio_buffer *p = nullptr;
            int size = sizeof(sizeof(void*));
            alsa_conf.hdmi_in_cache_queue->Get((unsigned char *)&p, size);
            free(p);
        }
        delete(alsa_conf.hdmi_in_cache_queue);
        alsa_conf.hdmi_in_cache_queue = nullptr;
    }
    if(alsa_conf.hdmi_in_read_queue) {
        while(alsa_conf.hdmi_in_read_queue->GetDataLen() > 0)
        {
            t_audio_buffer *p = nullptr;
            int size = sizeof(sizeof(void*));
            alsa_conf.hdmi_in_read_queue->Get((unsigned char *)&p, size);
            free(p);
        }
        delete(alsa_conf.hdmi_in_read_queue);
        alsa_conf.hdmi_in_read_queue = nullptr;
    }
    if(alsa_conf.usb_in_queue) {
        delete(alsa_conf.usb_in_queue);
        alsa_conf.usb_in_queue = nullptr;
    }

#if SET_THREAD_PRIORITY == 1
    pthread_attr_destroy(&attr_read_line_in_thread);
    pthread_attr_destroy(&attr_read_hdmi_in_thread);
#endif

    info("debug quit");
    return 0;
}
