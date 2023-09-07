/**
  ******************************************************************************
  * @file           : Decoder.h
  * @brief          : 解码接收，从mp4解出h264 h265帧数据后通过DecodeImp进行解码显示
  * @attention      : None
  * @version        : 1.0
  * @revision       : none
  * @date           : 2023/6/7
  * @author         : yangkun
  * @email          : xyyangkun@163.com
  * @company        : osee-dig.com.cn
  ******************************************************************************
  */

#ifndef RV11XX_PRJ_DECODER_H
#define RV11XX_PRJ_DECODER_H
#include <thread>
#include <mutex>
#include <queue>
#include <deque>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <string>

#include "rk_h264_decode.h"

#include "easymedia/rkmedia_api.h"
#include "easymedia/rkmedia_vdec.h"

class Decoder;
class Decoder {
public:

    explicit Decoder(const std::string& name);
    virtual  ~Decoder() = default;

    virtual void input(void *data, unsigned int size, unsigned long long timestamp) = 0;
    virtual void get_eof_data() = 0;
    std::string& getName();

    virtual void reinit() = 0;
private:
    std::string m_name;
};

struct s_video_tm
{
    int64_t time; //  relative time
    int64_t pts;  //  时间戳
};


class H264Decoder;
// 1080p 720p 和其它格式进行解码
class H264Decoder : public Decoder
{
public:
    //using Ptr = std::shared_ptr<H264Decoder>;
    explicit H264Decoder(AVRational _video_time_base);
    ~H264Decoder() override;

    void input(void *data, unsigned int size, unsigned long long timestamp) override;
    void get_eof_data() override;
    void reinit()override;
private:
    static int64_t _gettime_relative()
    {
        struct timespec ts = {0};
        // 获取开机后，相对于开机时的时间戳
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }
private:
    std::thread th;
    bool quit = false;
    void decode_proc();

    struct vpu_h264_decode decode{};
    int w = 0, h = 0;

    uint64_t _count = 0;



    std::mutex mtx;
    std::queue<MEDIA_BUFFER>  queue;
    std::condition_variable cond;

    struct s_video_tm prev_time={0,0};
    struct s_video_tm current_time={0,0};

    // 播放帧率控制
    int64_t first_video_pts = 0;
    int64_t video_real_start_time = 0; // 开始真实时间
    AVRational audio_time_base, video_time_base;
    unsigned long long _video_prev_timestampe = 0;// 上以帧视频的时间戳
    int64_t _video_time_left = 0;
    int video_timeval = 17;


    bool pause = false;
};


#endif //RV11XX_PRJ_DECODER_H
