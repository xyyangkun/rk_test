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
#include <zlog.h>
extern zlog_category_t * log_category;
extern int log_init();
extern void log_fini();

#ifndef ZLOG
#ifdef DEBUG
//红色表示错误信息
#define log_error(fmt,args ...)    do {\
    printf("\033[1;31m%s %s ==>%s(%d)" ": " fmt"\033[0;39m", __DATE__,__TIME__,__FUNCTION__, __LINE__, ## args);\
}while(0)
//绿色提示信息
#define log_info(fmt,args ...) do {\
    printf("\033[1;33m"fmt"\033[0;39m", ## args);\
}while(0)

#define log_trace_err(fmt, arg...) do {\
    fprintf(stderr, "\33[31;1m%s, %s-%d: "fmt"\33[0m\n", __FILE__, __func__, __LINE__, ##arg);\
}while(0)

#define log_print(fmt,args ...) do {\
    printf("%s(%d)" ": " fmt " ", __FUNCTION__, __LINE__, ## args);\
}while(0)

#else
#define log_print(fmt,args ...)
#define log_error(fmt,args ...)
#define log_info(fmt,args ...)
#define log_trace_err(fmt, arg...)
#endif

#else // ZLOG
    /// ZLOG的实现
#define log_error(fmt,args ...) do {\
    if(log_category)zlog_error(log_category, fmt, ##args ); \
}while(0)

#define log_print(fmt,args ...) do {\
    if(log_category)zlog_debug(log_category, fmt, ##args ); \
}while(0)

#define log_info(fmt,args ...) do {\
    if(log_category)zlog_info(log_category, fmt, ##args); \
}while(0)

#define comm_protocol_info(fmt,args ...) do {\
    if(log_category)zlog_info(log_category, fmt, ##args); \
}while(0)

#define log_trace_err(fmt, arg...) do {\
    if(log_category)zlog_error(log_category, fmt, ##args ); \
}while(0)


#endif // endof ZLOG

#endif//_ZLOG_API_H_
