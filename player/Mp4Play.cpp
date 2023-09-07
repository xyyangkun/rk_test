/**
  ******************************************************************************
  * @file           : Mp4Play.cpp
  * @brief          : 重写mp4播放，视频解码播放放在一块，不缓存解码后的数据
  * @attention      : None
  * @version        : 1.0
  * @revision       : none
  * @date           : 2023/5/16
  * @author         : yangkun
  * @email          : xyyangkun@163.com
  * @company        : osee-dig.com.cn
  ******************************************************************************
  */

#include <cassert>
#include <csignal>
#include "Mp4Play.h"
Mp4Play::Mp4Play()
{
    osee_info("Mp4Play call");
}
Mp4Play::~Mp4Play()
{
    osee_info("Mp4Play will exit!");
    // 关闭线程
    is_call_close=1;
    av_th.join();

    // 释放ffmpeg内存
    if(video_dec_ctx) {
        avcodec_free_context(&video_dec_ctx);
        video_dec_ctx = nullptr;
    }

    osee_info("audio_dec_ctx free!");
    if(audio_dec_ctx) {
        avcodec_free_context(&audio_dec_ctx);
        audio_dec_ctx = nullptr;
    }

    osee_info("fmt_ctx free!");
    if(fmt_ctx) {
        avformat_close_input(&fmt_ctx);
        fmt_ctx = nullptr;
    }

    osee_info("frame free!");
    if(frame){
        av_frame_free(&frame);
        frame = nullptr;
    }

    osee_info("h264bsfc free!");
    if(h264bsfc)
    {
        av_bitstream_filter_close(h264bsfc);
        h264bsfc = nullptr;
    }
    osee_info("h265bsfc free!");
    if(h265bsfc)
    {
        av_bitstream_filter_close(h265bsfc);
        h265bsfc = nullptr;
    }

    if(dec)
    {
        osee_info("Decoder will delete!");
        delete dec;
    }

}

/* Enable or disable frame reference counting. You are not supposed to support
 * both paths in your application but pick the one most appropriate to your
 * needs. Look for the use of refcount in this example to see what are the
 * differences of API usage between them. */
static int refcount = 0;

static int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec = nullptr;
    AVDictionary *opts = nullptr;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file \n",
                av_get_media_type_string(type));
        return ret;
    } else {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }

        /* Init the decoders, with or without reference counting */
        av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
        if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }

    return 0;
}

static int get_format_from_sample_fmt(const char **fmt,
                                      enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
            { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
            { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
            { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
            { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
            { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };
    *fmt = nullptr;

    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }

    fprintf(stderr,
            "sample format %s is not supported as output format\n",
            av_get_sample_fmt_name(sample_fmt));
    return -1;
}
extern "C" void set_vo(int w, int h);
int Mp4Play::Mp4Open(const char *file_path)
{
    osee_print("===================> file_path:%s\n", file_path);

    /* open input file, and allocate format context */
    if (avformat_open_input(&fmt_ctx, file_path, nullptr, nullptr) < 0) {
        osee_print("Could not open source file %s\n", file_path);
        return -1;
    }
    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        osee_error("Could not find stream information\n");
        return -2;
    }

    if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
        video_stream = fmt_ctx->streams[video_stream_idx];

        video_time_base = video_stream->time_base;

        video_interval = (int)(1000.0/av_q2d(video_stream->avg_frame_rate));

        /* allocate image where the decoded image will be put */
        width = video_dec_ctx->width;
        height = video_dec_ctx->height;
        pix_fmt = video_dec_ctx->pix_fmt;
        printf("video width:%d height:%d \n", width, height);

        if(video_dec_ctx->codec_id==AV_CODEC_ID_H264)
        {
            _video_dec_type = 1;
        }
        if(video_dec_ctx->codec_id==AV_CODEC_ID_HEVC)
        {
            _video_dec_type = 2;
        }
    }

    // set_vo(width, height);

    if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
        audio_stream = fmt_ctx->streams[audio_stream_idx];

        audio_time_base = audio_stream->time_base;

        video_interval = (int)(1000.0/av_q2d(video_stream->avg_frame_rate));

        enum AVSampleFormat sfmt = audio_dec_ctx->sample_fmt;
        int n_channels = audio_dec_ctx->channels;
        printf("channe:%d sfmt:%d\n", n_channels, sfmt);
    }

    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, file_path, 0);

    if (!audio_stream && !video_stream) {
        osee_error("Could not find audio or video stream in the input, aborting\n");
        ret = 1;
        //close_local_file();
        exit(-1);
        return -1;
    }


    frame = av_frame_alloc();
    if (!frame) {
        //fprintf(stderr, "Could not allocate frame\n");
        osee_error("could not allocalt frame\n");
        ret = AVERROR(ENOMEM);
        //close_local_file();
        exit(-1);
        return -2;
    }

    /* initialize packet, set data to nullptr, let the demuxer fill it */
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    av_init_packet(&pkt1);
    pkt1.data = nullptr;
    pkt1.size = 0;

    if(_video_dec_type == 1)
    {
        dec = new H264Decoder(video_time_base);
        h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb");
    }
    if(_video_dec_type == 2)
        h265bsfc =  av_bitstream_filter_init("hevc_mp4toannexb");
    assert(th_mux_read_proc == 0);
    // 创建线程读取文件, 并将音视频数据放入回调
    // ret = pthread_create(&th_mux_read_proc, nullptr, mux_read_proc, nullptr);
    av_th = std::thread(&Mp4Play::MuxReadProc, this);
    if(av_th.joinable() != true) {
        osee_error("error to create thread:%d errstr:%s\n", ret, strerror(errno));
        exit(1);
    }

    return 0;
}


int Mp4Play::decode_packet(int *got_frame, int cached)
{
    int ret = 0;
    int decoded = pkt.size;

    *got_frame = 0;

#if 0
    printf("pts:%ld dts:%ld   ===>  %f ==> %d\n",
			pkt.pts, pkt.dts,
			pkt.pts*av_q2d(video_stream->time_base),
			(int)(pkt.pts*av_q2d(video_stream->time_base)));
#endif
    _seek = (unsigned int)(pkt.pts*av_q2d(video_stream->time_base));

    if (pkt.stream_index == video_stream_idx) {

        if(_video_dec_type == 1)
        {
            // 得到h264/h265 视频帧数据，将视频帧数据传给回调函数
            // 两种解析MP4方法
            // https://blog.csdn.net/m0_37346206/article/details/94029945
            // https://blog.csdn.net/godvmxi/article/details/52875058
            // av_cb(buf, size, 1);
            int a = av_bitstream_filter_filter(h264bsfc,
                                               fmt_ctx->streams[video_stream_idx]->codec,
                                               nullptr,
                                               &pkt1.data,
                                               &pkt1.size,
                                               pkt.data,
                                               pkt.size,
                                               pkt.flags & AV_PKT_FLAG_KEY);
            /*
            printf("a=%d\n", a);
                printf("new_pkt.data=%p %p %d %d\n",
                    pkt.data, pkt1.data,
                    pkt.size, pkt1.size);
                    */
            if(a<0){
                printf("error to get bit stream\n");
                osee_error("error to get bit stream\n");
                av_freep(&pkt1.data);
                av_packet_free_side_data(&pkt1);
                av_packet_unref(&pkt1);
                //exit(-1);
                return -1;
            }
        }
        if(_video_dec_type == 2)
        {
            int a = av_bitstream_filter_filter(h265bsfc,
                                               fmt_ctx->streams[video_stream_idx]->codec,
                                               nullptr,
                                               &pkt1.data,
                                               &pkt1.size,
                                               pkt.data,
                                               pkt.size,
                                               pkt.flags & AV_PKT_FLAG_KEY);
            /*
            printf("a=%d\n", a);
                printf("new_pkt.data=%p %p %d %d\n",
                    pkt.data, pkt1.data,
                    pkt.size, pkt1.size);
                */
            if(a<0){
                printf("error to get bit stream\n");
                osee_error("error to get bit stream\n");
                //exit(-1);
                return -1;
            }
        }
#if 0
        if(a>=0){
		 av_free_packet(&pkt);
		 pkt.data = new_pkt.data;
		 pkt.size = new_pkt.size;
		 }
#endif

        //av_cb(pkt.data, pkt.size, 2, cb_handle);
        video_process(pkt1.data, pkt1.size, 2, pkt.pts, 0);

        // 释放内存
        av_freep(&pkt1.data);
        av_packet_free_side_data(&pkt1);
        av_packet_unref(&pkt1);
        //av_free_packet(&pkt1);
        pkt1.data = nullptr;
        pkt1.size = 0;

    } else if (pkt.stream_index == audio_stream_idx) {
        assert(audio_dec_ctx);
        /* decode audio frame */
        ret = avcodec_decode_audio4(audio_dec_ctx, frame, got_frame, &pkt);
        if (ret < 0) {
            char err_str[256] = { 0 };
            av_strerror(ret, err_str, sizeof(err_str));
            fprintf(stderr, "Error decoding audio frame (%s)\n", err_str);
            return ret;
        }
        /* Some audio decoders decode only part of the packet, and have to be
         * called again with the remainder of the packet data.
         * Sample: fate-suite/lossless-audio/luckynight-partial.shn
         * Also, some decoders might over-read the packet. */
        decoded = FFMIN(ret, pkt.size);

        if (*got_frame) {
            size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample((enum AVSampleFormat)frame->format);
#if 0
            printf("audio_frame%s n:%d nb_samples:%d pts:%s\n",
                   cached ? "(cached)" : "",
                   audio_frame_count++, frame->nb_samples,
                   av_ts2timestr(frame->pts, &audio_dec_ctx->time_base));

			printf("pts:%lld %lld %lld  time_base: num:%d, den:%d\n",
					frame->pts,
					frame->pkt_pts,
					frame->pkt_dts,
					audio_dec_ctx->time_base.num, audio_dec_ctx->time_base.den);
#endif

            /* Write the raw audio data samples of the first plane. This works
             * fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
             * most audio decoders output planar audio, which uses a separate
             * plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
             * In other words, this code will write only the first audio channel
             * in these cases.
             * You should use libswresample or libavfilter to convert the frame
             * to packed data. */
            //fwrite(frame->extended_data[0], 1, unpadded_linesize, audio_dst_file);
            // av_cb(frame->extended_data[0], unpadded_linesize, 1, cb_handle);
            audio_process((void *) frame, unpadded_linesize, 1, pkt.pts, 0);


        }
    }

    /* If we use frame reference counting, we own the data and need
     * to de-reference it when we don't use it anymore */
    if (*got_frame && refcount)
        av_frame_unref(frame);

    return decoded;
}

void Mp4Play::MuxReadProc()
{
    int ret;
    int got_frame;
    int count = 0;
    int start = 0;

    // 暂时时开打文件需要，开始再暂停
    if(_pause == 1)
    {
        //_pause = 0;
        _pause1 = 0;
        start = 1;
    }
    else
    {
        start = 0;
    }
    again:
    /* read frames from the file */
    while (av_read_frame(fmt_ctx, &pkt) >= 0 && is_call_close==0) {
        // 暂停逻辑
        while(_pause == 1 && _pause1 == 1 && is_call_close==0)
        {
            osee_print("in mux read proc _pause = %d\n", _pause);
            if(_next_frame == 1)
            {
                osee_print("in mux read proc _nex_frame = %d\n", _next_frame);
                //_next_frame = 0;
                // 推出pause 循环
                break;
            }
            usleep(100*1000);
            continue;
        }

        AVPacket orig_pkt = pkt;
        do {
            ret = decode_packet(&got_frame, 0);
            if (ret < 0)
                break;
            pkt.data += ret;
            pkt.size -= ret;
        } while (pkt.size > 0);
        av_packet_unref(&orig_pkt);
        count++;
        // 解码5帧后暂停
        if(start == 1 && count >= 10)
        {
            start = 0;
            _pause1 = 1;
        }
    }

    // 停止时显示最后一帧
    //if( is_call_close == 1 ) _pause = 1;
    // 停止时,mp4音频无输入

    if(dec) dec->get_eof_data();

    // seek 到0
    if(1 && is_call_close != 1 )
    {
        int seek = 0;
        int64_t startTime = (int64_t)seek * AV_TIME_BASE;
        int64_t target_time = av_rescale_q(startTime,AV_TIME_BASE_Q,video_stream->time_base);

        //ret = av_seek_frame(fmt_ctx, video_stream_idx, (int64_t)time*(AV_TIME_BASE),AVSEEK_FLAG_BACKWARD);
        //ret = av_seek_frame(fmt_ctx, video_stream_idx, target_time, AVSEEK_FLAG_BACKWARD);
        ret = avformat_seek_file(fmt_ctx, video_stream_idx, target_time, target_time, target_time, AVSEEK_FLAG_BACKWARD);
        if(ret < 0)
        {
            osee_error("ERROR in av_seek_frame:%d\n", ret);
            exit(1);
        }
        if(dec)
        {
            dec->reinit();
        }

        usleep(100*1000);
        goto again;
    }


    /* flush cached frames */

    pkt.data = nullptr;
    pkt.size = 0;
    do {
        decode_packet(&got_frame, 1);
    } while (got_frame);

    printf("Demuxing succeeded.\n");



    if (video_stream) {
        //printf("Play the output video file with the command:\n"
        //       "ffplay -f rawvideo -pix_fmt %s -video_size %dx%d %s\n",
        //       av_get_pix_fmt_name(pix_fmt), width, height,
        //       video_dst_filename);
    }
#if 1
    if (audio_stream) {
        enum AVSampleFormat sfmt = audio_dec_ctx->sample_fmt;
        int n_channels = audio_dec_ctx->channels;
        const char *fmt;

        if (av_sample_fmt_is_planar(sfmt)) {
            const char *packed = av_get_sample_fmt_name(sfmt);
            printf("Warning: the sample format the decoder produced is planar "
                   "(%s). This example will output the first channel only.\n",
                   packed ? packed : "?");
            sfmt = av_get_packed_sample_fmt(sfmt);
            n_channels = 1;
        }

        if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0)
            goto end;

        //printf("Play the output audio file with the command:\n"
        //       "ffplay -f %s -ac %d -ar %d %s\n",
        //       fmt, n_channels, audio_dec_ctx->sample_rate,
        //       audio_dst_filename);
    }
#endif

    end:
    av_free_packet(&pkt);
    av_free_packet(&pkt1);

    // 读取文件结束，通过回调通知上层，文件结束事件
    //assert(close_cb);
    //if(close_cb) {
    //    close_cb(is_call_close, cb_handle);
    //}
}

int Mp4Play::video_process(void* buf, unsigned int buf_size,
                           unsigned int type, unsigned long long timestamp,
                           long handle)
{
    // buf, buf_size为h264数据
    osee_info("---------------video buf size:%d, timestamp=%llu", buf_size, timestamp);

    if(dec)dec->input(buf, buf_size, timestamp);
    return 0;
}
int Mp4Play::audio_process(void* buf, unsigned int buf_size,
                           unsigned int type, unsigned long long timestamp,
                           long handle)
{
    // void *buf, unsigned int buf_size 为pcm数据
    osee_info("++++++++++++++++audio buf size:%d, timestamp=%llu", buf_size, timestamp);

    return 0;
}