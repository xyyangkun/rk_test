/******************************************************************************
 *
 *       Filename:  zlog_api.h
 *
 *    Description:  zlog api
 *
 *        Version:  1.0
 *        Created:  2022年02月15日 14时37分15秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef _ZLOG_API_H_
#define _ZLOG_API_H_
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /*  __cplusplus */
#include "zlog.h"
extern zlog_category_t * log_category;
extern int log_init();
extern void log_fini();

static int _dbg=0;
/// \brief 初始化是否要运行测试
/// \param void
/// \return void
static void _dbg_init(void)
{
#ifdef MODULE
    if (getenv("dbg") || getenv(MODULE))
#else
    if (getenv("dbg"))
#endif
    {
        _dbg=1;
    }
}

#define dbg(fmt, args...)\
    do {\
        if (_dbg){printf("[%s %d]"fmt, __FUNCTION__, __LINE__, ##args);}\
	}\
    while(0)

/*
    字体颜色
#30:黑
#31:红
#32:绿
#33:黄
#34:蓝色
#35:紫色
#36:深绿
#37:白色
        背景颜色
#40:黑
#41:深红
#42:绿
#43:黄色
#44:蓝色
#45:紫色
#46:深绿
#47:白色
    echo -e "\e[43;35m yk \e[0m hello word"
    比如上面的命令，43是背景颜色，35是字体颜色， \e[0m 是还原本色
*/
#define LOG_CLRSTR_NONE                 "\033[m"
#define LOG_CLRSTR_RED                  "\033[0;32;31m"
#define LOG_CLRSTR_GREEN                "\033[0;32;32m"
#define LOG_CLRSTR_BLUE                 "\033[0;32;34m"
#define LOG_CLRSTR_DARY_GRAY            "\033[1;30m"
#define LOG_CLRSTR_CYAN                 "\033[0;36m"
#define LOG_CLRSTR_PURPLE               "\033[0;35m"
#define LOG_CLRSTR_BROWN                "\033[0;33m"
#define LOG_CLRSTR_YELLOW               "\033[1;33m"
#define LOG_CLRSTR_WHITE                "\033[1;37m"

#define LOG_CHAR_RED                 "\033[1;31m"  /*背景黑色，红字,  比 \033[0;31m 看起来更亮*/
#define LOG_CHAR_CLEAR               "\033[0m"  /*清除格式*/
#define DEBUG
#ifndef ZLOG
#ifdef DEBUG
//红色表示错误信息
#define osee_error(fmt,args ...)    do {\
    printf("\033[1;31m%s %s ==>%s(%d)" ": " fmt"\033[0;39m", __DATE__,__TIME__,__FUNCTION__, __LINE__, ## args);\
}while(0)

#define osee_warn(fmt,args ...) do { \
    printf("\033[1;31m%s %s ==>%s(%d)" ": " fmt"\033[0;39m", __DATE__,__TIME__,__FUNCTION__, __LINE__, ## args);\
}while(0)

//绿色提示信息
#define osee_info(fmt,args ...) do {\
    printf("\033[1;33m" fmt "\033[0;39m", ## args);\
}while(0)

#define osee_trace_err(fmt, arg...) do {\
    fprintf(stderr, "\33[31;1m%s, %s-%d: "fmt"\33[0m\n", __FILE__, __func__, __LINE__, ##arg);\
}while(0)

#define osee_print(fmt,args ...) do {\
    printf("%s(%d)" ": " fmt " \n", __FUNCTION__, __LINE__, ## args);\
}while(0)

#else
#define osee_print(fmt,args ...)
#define osee_error(fmt,args ...)
#define osee_info(fmt,args ...)
#define osee_trace_err(fmt, arg...)
#endif

#else // ZLOG
    /// ZLOG的实现
#define osee_error(fmt,args ...) do { \
    if (_dbg)fprintf(stdout, LOG_CHAR_RED"%s %s ==>%s(%d)" ": " fmt LOG_CHAR_CLEAR"\n", __DATE__,__TIME__,__FUNCTION__, __LINE__, ## args); \
    if(log_category)zlog_error(log_category, LOG_CHAR_RED fmt LOG_CHAR_CLEAR, ##args ); \
}while(0)

#define osee_warn(fmt,args ...) do { \
    if (_dbg)fprintf(stdout, LOG_CLRSTR_YELLOW"%s %s ==>%s(%d)" ": " fmt LOG_CHAR_CLEAR"\n", __DATE__,__TIME__,__FUNCTION__, __LINE__, ## args); \
    if(log_category)zlog_warn(log_category, LOG_CLRSTR_YELLOW fmt LOG_CHAR_CLEAR, ##args ); \
}while(0)

#define osee_print(fmt,args ...) do {\
    if(log_category)zlog_debug(log_category, fmt, ##args ); \
}while(0)

#define osee_info(fmt,args ...) do { \
    if (_dbg)fprintf(stdout, LOG_CLRSTR_GREEN"%s %s ==>%s(%d)" ": " fmt LOG_CHAR_CLEAR"\n", __DATE__,__TIME__,__FUNCTION__, __LINE__, ## args); \
    if(log_category)zlog_info(log_category, LOG_CLRSTR_GREEN fmt LOG_CHAR_CLEAR, ##args); \
}while(0)

#define comm_protocol_info(fmt,args ...) do {\
    if(log_category)zlog_info(log_category, fmt, ##args); \
}while(0)

#define osee_trace_err(fmt, arg...) do {\
    if(log_category)zlog_error(log_category, fmt, ##args ); \
}while(0)

#define osee_trace(fmt, arg...) do {\
    fprintf(stdout, "%s %s ==>%s(%d)" ": " fmt "\n", __DATE__,__TIME__,__FUNCTION__, __LINE__, ## args); \
}while(0)


#endif // endof ZLOG

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /*  __cplusplus */
#endif//_ZLOG_API_H_
