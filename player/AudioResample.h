//
// Created by win10 on 2023/8/24.
// 转换音频采集率
//

#ifndef OSEE_LIVE_AUDIORESAMPLE_H
#define OSEE_LIVE_AUDIORESAMPLE_H
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
#include "libavcodec/avcodec.h"
#include <libavformat/avformat.h>
#include "libavformat/avio.h"

#include "libavutil/channel_layout.h"
#include "libavutil/md5.h"
#include "libavutil/opt.h"
#include "libavutil/samplefmt.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"

#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/time.h>

#include "libavdevice/avdevice.h"

#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


class AudioResample {
public:
    AudioResample();
    ~AudioResample();

    int init_swr(int input_channels, int input_sample_rate, AVSampleFormat input_sample_fmt = AV_SAMPLE_FMT_S16);
    int deinit_swr();
    int resample_swr(void *in, int in_size);
    int audio_resample(void *in, int samples);


private:
    AVAudioFifo *fifo = nullptr;

    bool b_swr_init = false;

    void *_swr_ctx = nullptr; // 格式转换

    int alsa_out_samples = 1024;

    int dst_nb_samples;
    int dst_nb_channels =0;

    int src_nb_samples;
    int src_nb_channels =0;

    AVFrame *_frame = NULL;
    uint8_t **dst_data = NULL;
    int dst_linesize = 0;

    uint8_t **src_data = NULL;
    int src_linesize = 0;

    int dst_rate = 48000;
    int src_rate = 48000;
    int max_dst_nb_samples;
    int64_t src_ch_layout = AV_CH_LAYOUT_STEREO, dst_ch_layout = AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_S16, dst_sample_fmt = AV_SAMPLE_FMT_S16;
};


#endif //OSEE_LIVE_AUDIORESAMPLE_H
