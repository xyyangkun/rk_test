//
// Created by win10 on 2023/8/24.
//

#include <cassert>
#include <memory>
#include "AudioResample.h"
#include "zlog_api.h"

AudioResample::AudioResample()
{

}

AudioResample::~AudioResample()
{
    osee_info("~AudioResample release!!\n");
    deinit_swr();
}



int AudioResample::init_swr(int input_channels, int input_sample_rate, AVSampleFormat input_sample_fmt/* = AV_SAMPLE_FMT_S16*/)
{
    int ret;
    struct SwrContext *swr_ctx;

    // 创建转换
    // audio resample
    {
        // 动态获取usb格式输入
        if(input_channels == 2)
            src_ch_layout = AV_CH_LAYOUT_STEREO;
        else
            src_ch_layout = AV_CH_LAYOUT_MONO;
        src_rate = input_sample_rate;
        src_sample_fmt = input_sample_fmt;

        /* create resampler context */
        swr_ctx = swr_alloc();
        if (!swr_ctx) {
            fprintf(stderr, "Could not allocate resampler context\n");
            osee_error("Could not allocate resampler context\n");
            ret = AVERROR(ENOMEM);
            exit(1);
        }

        /* set options */
        av_opt_set_int(swr_ctx, "in_channel_layout",    src_ch_layout, 0);
        osee_print("src rate:%d\n", src_rate);
        av_opt_set_int(swr_ctx, "in_sample_rate",       src_rate, 0);
        osee_print("src fmt:%d\n", src_sample_fmt);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);

        av_opt_set_int(swr_ctx, "out_channel_layout",    dst_ch_layout, 0);
        av_opt_set_int(swr_ctx, "out_sample_rate",       dst_rate, 0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

        /* initialize the resampling context */
        if ((ret = swr_init(swr_ctx)) < 0) {
            fprintf(stderr, "Failed to initialize the resampling context\n");
            osee_error("Failed to initialize the resampling context\n");
            ret = -1;
            exit(1);
        }
    }

    if (!(fifo = av_audio_fifo_alloc(src_sample_fmt,
                                     input_channels, input_sample_fmt))) {
        fprintf(stderr, "Could not allocate FIFO\n");
        return AVERROR(ENOMEM);
    }

    int frame_samples = alsa_out_samples;
    max_dst_nb_samples = dst_nb_samples = av_rescale_rnd(frame_samples, dst_rate, src_rate, AV_ROUND_UP);

    /* buffer is going to be directly written to a rawaudio file, no alignment */
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
    osee_print("dst_nb_channels = %d, dst_nb_samples=%d, dst_sample_fmt=%d\n", dst_nb_channels, dst_nb_samples, dst_sample_fmt);
    assert(dst_nb_channels == 2);
    // 根据格式分配内存
    ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels,
                                             dst_nb_samples, dst_sample_fmt, 1);

    if (ret < 0) {
        fprintf(stderr, "Could not allocate destination samples\n");
        osee_error("Could not allocate destination samples\n");
        exit(1);
    }

#if 0
    src_nb_samples = input_sample_rate;
    /* buffer is going to be directly written to a rawaudio file, no alignment */
    src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);
    ret = av_samples_alloc_array_and_samples(&src_data, &src_linesize, src_nb_channels,
                                             src_nb_samples, input_sample_fmt, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate srcination samples\n");
        osee_error("Could not allocate srcination samples\n");
        exit(1);
    }
#endif

    osee_print("debug");
    // 赋值handle
    _swr_ctx = swr_ctx;
    return 0;
}

int AudioResample::deinit_swr()
{
    osee_info("deinit_swr");
    if(_swr_ctx)
    {
        swr_free((struct SwrContext **)&_swr_ctx);
        _swr_ctx = nullptr;
    }

    if(dst_data)
    {
        av_freep(&dst_data[0]);
        av_freep(&dst_data);
        dst_data = nullptr;
    }

    if(src_data)
    {
        av_freep(&src_data[0]);
        //av_freep(src_data);
        src_data = nullptr;
    }


    if (fifo)
    {
        av_audio_fifo_free(fifo);
        fifo = nullptr;
    }

    return 0;
}


static inline char * _av_err2str(int errnum)
{
    char tmp[AV_ERROR_MAX_STRING_SIZE] = {0};
    return av_make_error_string(tmp, AV_ERROR_MAX_STRING_SIZE, errnum);
}

int AudioResample::resample_swr(void *in, int in_size)
{
    int ret;
    osee_print("dbg");
    osee_print("resample_swr size:%d", in_size);

    {
#if 1
        //int out_buf_size = 1024*2*2;
        // 可以直接从package包中获取数据，然后生成AVFrame 包，将数据放入AVFrame包中
        //int pkt_size = out_buf_size;//pkt.size;

        int frame_size = in_size;

        //assert(pkt_size> 0);
        //assert(_frame->nb_samples> 0);

        // 转换成48k双通道后入栈
        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(swr_get_delay((struct SwrContext *)_swr_ctx, src_rate) +
                                        frame_size, dst_rate, src_rate, AV_ROUND_UP);
        if (dst_nb_samples > max_dst_nb_samples) {
            av_freep(&dst_data[0]);
            av_freep(&dst_data);
            ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,
                                   dst_nb_samples, dst_sample_fmt, 1);
            if (ret < 0)
            {
                osee_error("error in av sample alloc!");
                exit(1);
            }
            max_dst_nb_samples = dst_nb_samples;
            osee_print("=========================> !! change!!\n");
        }

        char *_audio_buf[2];
        _audio_buf[0] = (char *)in;
        _audio_buf[1] = (char *)in+4096;
        osee_print("dbg dst_nb_samples=%d, frame_size=%d", dst_nb_samples, frame_size);
        osee_print("dbg");
        memset((void *)dst_data[0],0,  dst_nb_samples*2*2);
        osee_print("dbg");
        //  先转换为48000 s16格式
        /* convert to destination format */
        ret = swr_convert((struct SwrContext *)_swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)_audio_buf, frame_size);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            osee_error("Error while converting\n");
            exit(1);
        }
        osee_print("dbg");
        int dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
                                                     ret, dst_sample_fmt, 1);
        if (dst_bufsize < 0) {
            fprintf(stderr, "Could not get sample buffer size\n");
            osee_error("Could not get sample buffer size\n");
            exit(1);
        }

        osee_print("dst_linesize:%d\n", dst_linesize);
#endif
    }
    return 0;
}

extern "C" void audio_recv1(void *buf, unsigned int size, long handle);
extern "C" int set_lose(int lose_chn,int value);
extern "C" void ndi_audio_write(void *buf, int size);
int AudioResample::audio_resample(void *in, int samples) {
    int ret;
    void *data[2];
    // 每个采样点点的字节数
    int bytes_per_sample = av_get_bytes_per_sample(src_sample_fmt);
    // 音频通道数
    int chns = av_get_channel_layout_nb_channels(src_ch_layout);

#if 0
    if(0)
    {
        static std::shared_ptr<FILE> fpa;
        if (!fpa) {
            osee_print("dbg audio_frame.p_data=%p", in);
            fpa.reset(fopen("flat.pcm", "wb"), [](FILE *fp) { fclose(fp); });
        }
        if (fpa) {
            //unsigned char *p = (unsigned char *)in;
            //osee_print("dbg audio_frame.p_data=%p %02x %02x %02x %20x", in, p[0], p[1], p[2], p[3]);
            fwrite(in, 2048 * 2 * 4, 1, fpa.get());
        }
    }
#endif
    if(av_sample_fmt_is_planar(src_sample_fmt))
    {
        //osee_print("input audio is planar!, samples=%d bytes_per_sample = %d , chns=%d",
        //           samples, bytes_per_sample, chns);
        data[0] = in;
        data[1] = (char *)in + samples/2 * bytes_per_sample;
        //osee_print(" data = %p %p len = %d\n", data[0], data[1]);

        int len = samples/2 * bytes_per_sample;
        //memcpy(src_data[0], in, len);
        //memcpy(src_data[1], in + len, len);
    }
    else
    {
        data[0] = in;
    }

    int nb_samples = samples;

#if 0
    // 将数据写入fifo数据
    ret =  av_audio_fifo_write(fifo, data, nb_samples);
    if(ret != ret)
    {
        osee_error("error in write audio fifo!");
        exit(1);
    }

    int size = av_audio_fifo_size(fifo);
#endif

    int frame_size = nb_samples;//in_size;
    //assert(pkt_size> 0);
    //assert(_frame->nb_samples> 0);

    // 转换成48k双通道后入栈
    /* compute destination number of samples */
    dst_nb_samples = av_rescale_rnd(swr_get_delay((struct SwrContext *)_swr_ctx, src_rate) +
                                    frame_size, dst_rate, src_rate, AV_ROUND_UP);
    if (dst_nb_samples > max_dst_nb_samples) {
        av_freep(&dst_data[0]);
        ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,
                               dst_nb_samples, dst_sample_fmt, 1);
        if (ret < 0)
        {
            osee_error("error in av sample alloc!");
            exit(1);
        }
        max_dst_nb_samples = dst_nb_samples;
        osee_print("=========================> !! change!!max_dst_nb_samples=%d, %d\n",
                   max_dst_nb_samples, dst_nb_samples);
    }

    //osee_print(" data = %p %p\n", data[0], data[1]);
    //  先转换为48000 s16格式
    /* convert to destination format */
    ret = swr_convert((struct SwrContext *)_swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)data, frame_size);
    if (ret < 0) {
        fprintf(stderr, "Error while converting\n");
        osee_error("Error while converting\n");
        exit(1);
    }
    //osee_print("dbg ret=%d", ret);
    int dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
                                                 ret, dst_sample_fmt, 1);
    if (dst_bufsize < 0) {
        fprintf(stderr, "Could not get sample buffer size\n");
        osee_error("Could not get sample buffer size\n");
        exit(1);
    }

    // ndi开启就应该有音频
    //set_lose(0,1);
    //audio_recv1((void *)dst_data, (unsigned int)dst_linesize/4, (long)0);
    set_lose(3,1);
    ndi_audio_write((void *)dst_data, (unsigned int)dst_linesize/4);

    //static std::shared_ptr<FILE> fpa = std::shared_ptr<FILE>(fopen("sss.pcm", "rw"), [](FILE *fp){fclose(fp);});
    //fwrite(dst_data[0], dst_linesize, 1, fpa.get());

    return 0;
}