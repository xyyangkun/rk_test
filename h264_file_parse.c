/******************************************************************************
 *
 *       Filename:  h264_file_parse.c
 *
 *    Description:  读取h264文件，获取nalu单元
 *
 *        Version:  1.0
 *        Created:  2022年12月02日 14时10分44秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include "h264_file_parse.h"

int yk_h264_parse(tyk_h264_parse *parse)
{
    FILE *fp =NULL;
    uint8_t* pbuf = NULL;
    bool bFindStart, bFindEnd;
    int i,  start = 0;
    uint32_t s32UsedBytes = 0, s32ReadLen = 0;

	uint32_t biggst_size = 0;

	uint64_t file_size;

    if(parse->file_name != 0)
    {
        fp = fopen(parse->file_name, "rb");
        if(fp == NULL)
        {
            printf("can't open file %s \n", parse->file_name);
			return -1;
        }
    }

	fseek(fp, 0, SEEK_END);
	
	// 获取文件大小
	file_size = ftell(fp);
	

    printf("yk_h264_parse stream file:%s, bufsize: %d\n", 
    parse->file_name, parse->mini_buf_size);

    pbuf = malloc(parse->mini_buf_size);
    if(pbuf == NULL)
    {
        printf("error can't alloc %d\n", parse->mini_buf_size);
        fclose(fp);
        return -1;
    }     
    fflush(stdout);
	
    while (1)
    {
        if (parse->is_run == false)
        {
            break;
        }
        
		// 读取h264 帧数据
        {
            bFindStart = false;  
            bFindEnd   = false;
            fseek(fp, s32UsedBytes, SEEK_SET);
            s32ReadLen = fread(pbuf, 1, parse->mini_buf_size, fp);
            if (s32ReadLen == 0)
            {
                if (parse->bloop)
                {
                    s32UsedBytes = 0;
                    fseek(fp, 0, SEEK_SET);
                    s32ReadLen = fread(pbuf, 1, parse->mini_buf_size, fp);
                }
                else
                {
					printf("yk debug read all file will break;!\n");
                    break;
                }
            }
         			
            for (i=0; i<s32ReadLen-8; i++)
            {
                int tmp = pbuf[i+3] & 0x1F;
                if (  pbuf[i] == 0 && pbuf[i+1] == 0 && pbuf[i+2] == 1 && 
                       (
                           ((tmp == 5 || tmp == 1) && ((pbuf[i+4]&0x80) == 0x80)) ||
                           (tmp == 20 && (pbuf[i+7]&0x80) == 0x80)
                        )
                   )            
                {
                    bFindStart = true;
                    i += 8;
					printf("++++++++++++++++++++++++ found start!, i=%d\n", i);
                    break;
                }
            }

            for (; i<s32ReadLen-8; i++)
            {
                int tmp = pbuf[i+3] & 0x1F;
                if (  pbuf[i  ] == 0 && pbuf[i+1] == 0 && pbuf[i+2] == 1 &&
                            (
                                  tmp == 15 || tmp == 7 || tmp == 8 || tmp == 6 || 
                                  ((tmp == 5 || tmp == 1) && ((pbuf[i+4]&0x80) == 0x80)) ||
                                  (tmp == 20 && (pbuf[i+7]&0x80) == 0x80 )
                              )
                   )                   
                {					
                    bFindEnd = true;
					printf("++++++++++++++++++++++++ found end! i=%d\n", i);
                    break;
                }
            }

            if(i > 0) s32ReadLen = i;
            if (bFindStart == false)
            {
                printf("error can not find start code!s32ReadLen %d, s32UsedBytes %d. \n", 
					           s32ReadLen, s32UsedBytes);
				// yk debug add, 没有找到头或者尾部，应该退出
				printf("error h264 not find head!, will exit\n");
				exit(1);
            }

            else if (bFindEnd == false && 
					file_size != s32ReadLen + s32UsedBytes/*排除读取到文件结尾*/)
            {
                s32ReadLen = i+8;
				// yk debug add, 没有找到头或者尾部, 应该是到文件尾部了
				printf("\error h264 not find tail!, will exit, s32ReadLen=%d ,file_size=%d\n", s32ReadLen, file_size);
				uint32_t i = start + s32ReadLen - 4;
				printf("%02x %02x %02x %02x    %02x %02x %02x %02x\n",
						pbuf[i+0], pbuf[i+1], pbuf[i+2], pbuf[i+3],
						pbuf[i+4], pbuf[i+5], pbuf[i+6], pbuf[i+7]);

				//exit(1);
            }
            
        }
		/*	
        stStream.pu8Addr = pbuf + start;
        stStream.u32Len  = s32ReadLen; 
		*/
       if(parse->handler)
	   {
			parse->handler(parse->param, pbuf + start, s32ReadLen);
			if (s32ReadLen > biggst_size)
				biggst_size = s32ReadLen;
	   }
        
        printf("parse One Frame,read byte=%d start=%d, readlen=%d\n",
				s32UsedBytes, start, s32ReadLen);
        fflush(stdout);   
        
        {
            s32UsedBytes = s32UsedBytes + s32ReadLen + start;			
        }
    }

    
    //printf("SAMPLE_TEST:send steam thread %d return ...\n", pstVdecThreadParam->s32ChnId);
	printf("found biggest_size= %u\n", biggst_size);
    fflush(stdout);
    if (pbuf != NULL)
    {
        free(pbuf);
    }
    fclose(fp);
	
    return 0;
}




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
