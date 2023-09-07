/******************************************************************************
 *
 *       Filename:  hello.c
 *
 *    Description:  test 
 *
 *        Version:  1.0
 *        Created:  2023年03月01日 13时59分09秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <stdio.h>
#include <signal.h>
#include <execinfo.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <memory>
#include "Mp4Play.h"

static bool quit = false;
// 打印栈信息
//#define USE_PBT
#ifdef USE_PBT
static void print_backtrace(void)
{
#define BCACKTRACE_DEEP 100
    void *array[BCACKTRACE_DEEP];
    int size, i;
    char **strings;
    size = backtrace(array, BCACKTRACE_DEEP);
    fprintf(stderr, "\nBacktrace (%d deep):\n", size);
    strings = backtrace_symbols(array, size);
    if (strings == NULL)
    {
        fprintf(stderr, "%s: malloc error\n", __func__);
        return;
    }
    for(i = 0; i < size; i++)
    {
        fprintf(stderr, "%d: %s\n", i, strings[i]);
    }
    free(strings);
}
#endif
static void sigterm_handler(int sig) {
    fprintf(stderr, "******************signal %d, SIGUSR1 = %d\n", sig, SIGUSR1);
#ifdef USE_PBT
    //if(SIGINT != sig)
    {
        print_backtrace();
    }
#endif

    quit = true;
}

zlog_category_t * log_category = NULL;
#define zlog_path "/userdata/osee_live/zlog.conf"

int main(int argc, char *argv[])
{
    // 将配置文件复制到 /userdata下面
    system("mkdir /userdata/osee_live/");
    system("cp /oem/osee_live/zlog.conf /userdata/osee_live/");
    if(zlog_init(zlog_path)) {
        printf("ERROR in zlog_init\n");
        zlog_fini();
        exit(1);
    }
    log_category = zlog_get_category("zlog");
    if(!log_category)
    {
        fprintf(stderr, "============================> Error in get comm protcol category\n");
        zlog_fini();
    }


    printf("%s initial finish\n", __func__);
    signal(SIGINT, sigterm_handler);
#ifdef USE_PBT
    signal(SIGILL, sigterm_handler);    /* Illegal instruction.  */
    signal(SIGABRT, sigterm_handler);   /* Abnormal termination.  */
    signal(SIGFPE, sigterm_handler);    /* Erroneous arithmetic operation.  */
    signal(SIGSEGV, sigterm_handler);   /* Invalid access to storage.  */
    signal(SIGTERM, sigterm_handler);   /* Termination request.  */
    /* Historical signals specified by POSIX. */
    signal(SIGBUS, sigterm_handler);    /* Bus error.  */
    signal(SIGSYS, sigterm_handler);    /* Bad system call.  */
    signal(SIGPIPE, sigterm_handler);   /* Broken pipe.  */
    signal(SIGALRM, sigterm_handler);   /* Alarm clock.  */
    /* New(er) POSIX signals (1003.1-2008, 1003.1-2013).  */
    signal(SIGXFSZ, sigterm_handler);   /* File size limit exceeded.  */
#endif

    std::shared_ptr<Mp4Play> mp4;
    mp4.reset(new Mp4Play, [](Mp4Play *p){delete p;});
    const char *path = "test.mp4";
    mp4->Mp4Open(path);

    while (!quit) {
        usleep(500000);
    }

    printf("%s %d===================>\n", __FUNCTION__, __LINE__);
    return 0;
}
