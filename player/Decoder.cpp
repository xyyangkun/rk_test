/**
  ******************************************************************************
  * @file           : Decoder.cpp
  * @brief          : None
  * @attention      : None
  * @version        : 1.0
  * @revision       : none
  * @date           : 2023/6/7
  * @author         : yangkun
  * @email          : xyyangkun@163.com
  * @company        : osee-dig.com.cn
  ******************************************************************************
  */

#include <unistd.h>
#include <cassert>
#include <libavutil/rational.h>
#include "easymedia/rkmedia_api.h"
//#include "easymedia/rkmedia_vdec.h"
#include "Decoder.h"
#include "zlog_api.h"

static int mp4_dbg = 0;
static void mp4_dbg_init()
{
    if (getenv("mp4_dbg"))

    {
        mp4_dbg = 1;
    }
}
#define mp4_err(fmt,args ...) do {\
    if (mp4_dbg)fprintf(stdout, LOG_CHAR_RED"%s %s ==>%s(%d)" ": " fmt LOG_CHAR_CLEAR"\n", __DATE__,__TIME__,__FUNCTION__, __LINE__, ## args); \
    if(mp4_dbg && log_category)zlog_error(log_category, LOG_CHAR_RED fmt LOG_CHAR_CLEAR, ##args ); \
}while(0)
#define mp4_info(fmt,args ...) do {\
    if (mp4_dbg)fprintf(stdout, LOG_CHAR_RED"%s %s ==>%s(%d)" ": " fmt LOG_CHAR_CLEAR"\n", __DATE__,__TIME__,__FUNCTION__, __LINE__, ## args); \
    if(mp4_dbg && log_category)zlog_error(log_category, LOG_CHAR_RED fmt LOG_CHAR_CLEAR, ##args ); \
}while(0)
#define mp4_dbg(fmt,args ...) do {\
    if(mp4_dbg && log_category)zlog_debug(log_category, fmt, ##args ); \
}while(0)

Decoder::Decoder(const std::string& name)
{
    m_name = name;
    osee_print("m_name=%s", m_name.c_str());
    mp4_dbg_init();
}

std::string& Decoder::getName()
{
    return m_name;
}


H264Decoder::H264Decoder(AVRational _video_time_base): Decoder("H264")
{
    video_time_base =  _video_time_base;
    memset(&decode, 0, sizeof(decode));
    w = 1920;
    h = 1080;
    osee_print("debug");
    int ret = vpu_decode_h264_init(&decode, w, h);
    if(ret != 0)
    {
        osee_error("error in vpu h264 decode init! ret = %d\n", ret);
        exit(1);
    }
    osee_print("debug");


    quit = false;
    // 开启线程
    th = std::thread(&H264Decoder::decode_proc, this);

}

H264Decoder::~H264Decoder()
{
    MEDIA_BUFFER mb;

    osee_info("H264Decoder will deconstruct");
    quit = true;
    osee_print("debug");
    th.join();

    osee_print("debug");
    {
        std::unique_lock<std::mutex> lck(mtx);
        cond.notify_all();
    }


    osee_print("debug");
    {
        std::unique_lock<std::mutex> lck(mtx);
        while(!queue.empty())
        {
            mb = queue.front();
            queue.pop();
            RK_MPI_MB_ReleaseBuffer(mb);
        }
    }

    osee_print("debug");
    vpu_decode_h264_done(&decode);
}

void H264Decoder::input(void *data, unsigned int data_size, unsigned long long timestamp) {
    int ret;
    MEDIA_BUFFER mb;
    int size;
    void *mb_array[10];
    if(data_size <= 0)
    {
        osee_error("ERROR: Buffer get null buffer... data_size = %d\n", data_size);
        return ;
    }
    mb = RK_MPI_MB_CreateBuffer(data_size, RK_FALSE, 0);
    if (!mb) {
        osee_error("ERROR: Buffer get null buffer... data_size = %d\n", data_size);
        exit(1);
    }
    memcpy(RK_MPI_MB_GetPtr(mb), data, data_size);
    timestamp = timestamp * 1000;
    RK_MPI_MB_SetTimestamp(mb, timestamp);

    mp4_err("==================> mb_time = %lld", timestamp);


    ret = vpu_decode_h264_doing(&decode, mb, &mb_array[0], (int *)&size);
    if(ret !=0 )
    {
        printf("ERROR:decode!ret=%d\n", ret);
        exit(1);
    }

    // 解码出数据
    if(size > 0) {
        // 遍历解码后的数据，进行处理
        // for (int i = size-1; i >=0 ; i--) {
        for (int i = 0; i < size; i++) {
            _count++;
            MEDIA_BUFFER _mb = (MEDIA_BUFFER) mb_array[i];
            if (_mb == nullptr) {
                osee_error("error in get _mb null!, will exit\n");
                exit(1);
            }

            static long long /*pts_old = 0, */pts_new = 0;
            pts_new = RK_MPI_MB_GetTimestamp(_mb);
            mp4_err(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>14336000=%d get dec pts:%lld\n", size, pts_new);
            // 输出宽高

            MB_IMAGE_INFO_S stImageInfo = {0};
            ret = RK_MPI_MB_GetImageInfo(_mb, &stImageInfo);
            if (ret) {
                osee_error("Get image info failed! ret = %d\n", ret);
                RK_MPI_MB_ReleaseBuffer(_mb);
                exit(0);
            }

            osee_print("Get Frame:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
                       "timestamp:%lld, ImgInfo:<wxh %dx%d, fmt 0x%x>\n",
                       RK_MPI_MB_GetPtr(_mb), RK_MPI_MB_GetFD(_mb), RK_MPI_MB_GetSize(_mb),
                       RK_MPI_MB_GetModeID(_mb), RK_MPI_MB_GetChannelID(_mb),
                       RK_MPI_MB_GetTimestamp(_mb), stImageInfo.u32Width,
                       stImageInfo.u32Height, stImageInfo.enImgType);


            // 将解码后的数据放到队列中
            {
                std::unique_lock<std::mutex> lck(mtx);
                queue.push(_mb);
            }
        }
    }

    // 硬件解码器也需要时间解码，等待7ms输入下一帧数据进行解码
    usleep(7000);

    // 释放空间
    ret = RK_MPI_MB_ReleaseBuffer(mb);
    if (ret != 0) {
        osee_error("error in release dec buff!\n");
        exit(1);
    }

    int queue_size;
    {
        std::unique_lock<std::mutex> lck(mtx);
        queue_size = queue.size();
        if (queue_size > 0) {
            cond.notify_all();
        }
        //queue.push(mb);
    }
    // pthread_yield();

    // 队列中数据过多，等待
    if(queue_size > 5)
    {
        std::unique_lock<std::mutex>  lock(mtx);
        cond.wait(lock);
    }
}
// 定义最小的int64 -2*63 = -9223372036854775808
#define MIN_INT64 -9223372036854775808

void H264Decoder::get_eof_data()
{
    int ret;
    MEDIA_BUFFER mb;
    int size;
    void *mb_array[10];
    ret = vpu_decode_h264_doing_last(&decode, &mb_array[0], (int *)&size);
    if(ret !=0 )
    {
        printf("ERROR:decode!ret=%d\n", ret);
        exit(1);
    }

    // 解码出数据
    if(size > 0) {
        // 遍历解码后的数据，进行处理
        // for (int i = size-1; i >=0 ; i--) {
        for (int i = 0; i < size; i++) {
            _count++;
            MEDIA_BUFFER _mb = (MEDIA_BUFFER) mb_array[i];
            if (_mb == nullptr) {
                osee_error("error in get _mb null!, will exit\n");
                exit(1);
            }

            static long long /*pts_old = 0, */pts_new = 0;
            pts_new = RK_MPI_MB_GetTimestamp(_mb);
            mp4_err(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>14336000=%d get dec pts:%lld\n", size, pts_new);
            // 输出宽高

            MB_IMAGE_INFO_S stImageInfo = {0};
            ret = RK_MPI_MB_GetImageInfo(_mb, &stImageInfo);
            if (ret) {
                osee_error("Get image info failed! ret = %d\n", ret);
                RK_MPI_MB_ReleaseBuffer(_mb);
                exit(0);
            }

            osee_print("Get Frame:ptr:%p, fd:%d, size:%zu, mode:%d, channel:%d, "
                       "timestamp:%lld, ImgInfo:<wxh %dx%d, fmt 0x%x>\n",
                       RK_MPI_MB_GetPtr(_mb), RK_MPI_MB_GetFD(_mb), RK_MPI_MB_GetSize(_mb),
                       RK_MPI_MB_GetModeID(_mb), RK_MPI_MB_GetChannelID(_mb),
                       RK_MPI_MB_GetTimestamp(_mb), stImageInfo.u32Width,
                       stImageInfo.u32Height, stImageInfo.enImgType);


            // 将解码后的数据放到队列中
            {
                std::unique_lock<std::mutex> lck(mtx);
                queue.push(_mb);
            }
        }
    }
    {
        std::unique_lock<std::mutex> lck(mtx);
        auto queue_size = queue.size();
        if (queue_size > 0) {
            cond.notify_all();
        }
        //queue.push(mb);
    }
}
void H264Decoder::reinit()
{

    // 等待将解码缓存的数据播放完成
    usleep(50*1000);

    // 播放显示等待
    pause = true;
    // 清空队列中的数据
    {
        std::unique_lock<std::mutex> lck(mtx);
        while(!queue.empty())
        {
            auto mb = queue.front();
            queue.pop();
            RK_MPI_MB_ReleaseBuffer(mb);
        }
    }


#if 0
    // 通过release 解码器，在重新初始化解码器，释放最后缓存的解码数据
    vpu_decode_h264_done(&decode);
    int ret = vpu_decode_h264_init(&decode, w, h);
    if(ret != 0)
    {
        osee_error("error in vpu h264 decode init! ret = %d\n", ret);
        exit(1);
    }
    osee_print("debug");
#endif

    //usleep(50*1000);
    // 初始化一些值
    first_video_pts = MIN_INT64;
    video_real_start_time = 0;
    _video_time_left = 0;
    video_timeval = 17;
    prev_time={0,0};
    current_time={0,0};

    pause = false;
};
void H264Decoder::decode_proc()
{
    int ret;

    int size;
    MEDIA_BUFFER mb;

    bool b_wait = false;



    first_video_pts = MIN_INT64;

    int64_t mb_time = 0;

    // 循环接收数据，解码显示
    while(quit == false)
    {
        if(pause)
        {
            usleep(20*1000);
            continue;
        }
        if(b_wait)
        {
            usleep(10*1000);
            b_wait = false;
        }
        int video_queue_size;
        {
            std::unique_lock<std::mutex> lock(mtx);
            video_queue_size = queue.size();
            if(video_queue_size < 2)
            {
                cond.notify_all();
            }

            // 队列中没有数据，等待再获取
            if (video_queue_size <= 0) {
                osee_warn("=============================>queue size is 0!!!");
                b_wait = true;
                continue;
            }

            mb = queue.front();
            queue.pop();
        }


        // 2. 获取时间戳
        mb_time = RK_MPI_MB_GetTimestamp(mb);
        mp4_err("==================> mb_time = %llu", mb_time);
        current_time.pts = mb_time;
        current_time.time = _gettime_relative();

        // 调试获取数据,查看其时间戳是否正常
#if 0
        {
            RK_MPI_MB_ReleaseBuffer(mb);
            usleep(21*1000);
            continue;
        }
#endif

        if(first_video_pts == MIN_INT64)
        {
            first_video_pts = mb_time;
            mp4_err("==================> mb_time = %llu", mb_time);
            video_real_start_time = _gettime_relative();
        }

        // !!! 根据计算的时间戳，给视频帧添加新的时间戳，这样编码后的视频帧就会带有时间戳

        auto time_base = video_time_base;
        // 计算上一帧时间间隔
#if 1
        int64_t t1=0,t2=0;

        _video_time_left=0;
        // video_speed = _mp4_get_speed();
        if(prev_time.time == 0){
            _video_time_left = video_timeval; // p60 第一次默认为17ms
        }else {
            // _video_time_left = (current_time.pts - prev_time.pts);// - (current_time.time-prev_time.time);// ) * av_q2d(time_base);

#if 0
            // 当前时间戳-开始时间戳 - （当前真时时间-开始播放时间)
            _video_time_left = (current_time.time - video_real_start_time/*audio_real_start_time*/) -
             int64_t(av_q2d(time_base)* (current_time.pts-first_video_pts) *1000);
#else
            t1 = int64_t(av_q2d(time_base)* 1000.0 * (current_time.pts - first_video_pts));  // 应该延迟
            t2 = (current_time.time - video_real_start_time)*1000;  /// 真实时间
            _video_time_left =  t1 - t2;
#endif
            mp4_info("-----------------------------------------------------   %lld %lld %lld -->%lld %lld %lld %lld\n",
                     _video_time_left, t1, t2,
                     current_time.pts, first_video_pts, current_time.time, video_real_start_time);
        }


        // 没有足够的时间
        if(_video_time_left < 0)
        {
            // 源端发送数据太慢，造成没有足够的视频帧
            // 调试视频播放速度使用
            mp4_err("error in video time left !!!_video_time_left=%lld video_size=%d", _video_time_left, video_queue_size);

            //_video_time_left = 20;
            //int st = 17000;//-(int)(_video_time_left);
            //osee_error("error in video time left !!!st  %d\n", st);
            //usleep(st);

            //_video_time_left = -_video_time_left;
            _video_time_left = 0;
        }
        // 时间太大时，直接赋值默认值
        if(_video_time_left >50*1000)
        {
            // 源端发来视频帧过多，或者播放的太慢，比如hdmi输出为1080p30时，播放1080p60视频
            // 应该丢帧
            mp4_err("warnning video left time > 50,_video_time_left = %lld video_size=%d", _video_time_left, video_queue_size);
            //_video_time_left = 50*1000;
        }

        if(_video_time_left >100*1000)
        {
            mp4_err("warnning video left time > 50,_video_time_left = %lld video_size=%d", _video_time_left, video_queue_size);
            // 大于100ms直接丢帧，有可能是在seek
            // 丢帧追赶视频帧
            {
                // 过多的数据释放掉
                RK_MPI_MB_ReleaseBuffer(mb);
            }
            continue;
        }

        // 将当前时间存储到上一次的时间
        prev_time = current_time;

        // pthread_yield();
        //延时
        // usleep(_video_time_left);
    osee_info("========================================> _video_time_left:%lld t1=%lld, t2=%lld\n",
              _video_time_left, t1, t2);
#endif



        assert(mb != NULL);

#if 1
        osee_print("-----------------------------------> send vo!\n");
        // 送显
        ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_VO, 0, mb);
        if (ret != 0) {
            osee_error("error to send _mb to vo display!, will exit\n");
            exit(0);
        }
#endif

#if 0
            usleep(17*1000);
#else
        mp4_info("-----------------------------------------------------   _video_time_left=%lld\n", _video_time_left);
        usleep(_video_time_left);
#endif

        RK_MPI_MB_ReleaseBuffer(mb);
    }
}