/**
  ******************************************************************************
  * @file           : Mp4Play.h
  * @brief          : 重写mp4播放，mp4 文件demux视频格式为h264, 音频格式为pcm,统一转化为48k 16位立体志
  *                     h264视频解码播放放在一块，不缓存解码后的数据.
  * @attention      : None
  * @version        : 1.0
  * @revision       : none
  * @date           : 2023/5/16
  * @author         : yangkun
  * @email          : xyyangkun@163.com
  * @company        : osee-dig.com.cn
  ******************************************************************************
  */

#ifndef RV11XX_PRJ_MP4PLAY_H
#define RV11XX_PRJ_MP4PLAY_H

extern "C" {
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
}

#include <cstring>
#include <cerrno>
#include <pthread.h>
#include <thread>
//#include "demux.h"
#include "zlog_api.h"
#include "Decoder.h"

class Mp4Play {
public:
    Mp4Play();
    ~Mp4Play();
    int Mp4Open(const char *file_path);

private:
    Decoder *dec = nullptr;
private:
    AVFormatContext *fmt_ctx = nullptr;
    int video_stream_idx = -1, audio_stream_idx = -1;
    AVCodecContext *video_dec_ctx = nullptr, *audio_dec_ctx=nullptr;

    AVStream *video_stream = nullptr, *audio_stream = nullptr;

    AVRational audio_time_base = {0}, video_time_base= {0};

    int video_interval = 0;

    // demux
    AVFrame *frame = nullptr;
    AVPacket pkt = {nullptr};
    AVPacket pkt1= {nullptr};

    AVBitStreamFilterContext* h264bsfc = nullptr;
    AVBitStreamFilterContext* h265bsfc = nullptr;

    int ret = 0;
    int width = 0, height = 0;
    enum AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;


    int _video_dec_type  = 0;  // 1 h264; 2 h265


    pthread_t th_mux_read_proc = 0;


    // 创建线程读取文件, 并将音视频数据放入回调
    bool av_quit = false;
    std::thread av_th;
    void MuxReadProc();

    // 状态控制
    int _pause = 0;
    int _pause1 = 0;
    unsigned int _seek = 0;

    int _speed = 1;
    // 暂停时seek 为0 时特殊处理
    unsigned int _seek_state = 0;

    int _next_frame = 0; // 当暂停时进行下一帧，或者暂停时拖动进度条。需要调用下一帧进行解码显示
    unsigned long long _next_frame_time = 0;

    // 关闭调用
    int is_call_close = 0;

private:
    int decode_packet(int *got_frame, int cached);
    // ffmpeg解析出h264 ,音频解码出pcm
    int video_process(void* buf, unsigned int buf_size,
                      unsigned int type, unsigned long long timestamp,
                      long handle);
    int audio_process(void* buf, unsigned int buf_size,
                      unsigned int type, unsigned long long timestamp,
                      long handle);
};


#endif //RV11XX_PRJ_MP4PLAY_H
