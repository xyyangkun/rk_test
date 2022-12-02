/******************************************************************************
 *
 *       Filename:  h264_file_parse.h
 *
 *    Description:  读取h264 文件，获取nalu单元
 *
 *        Version:  1.0
 *        Created:  2022年12月02日 14时10分05秒
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
#include <stdint.h>
#include <stdbool.h>


typedef struct syk_h264_parse
{
    char file_name[100];
    uint32_t mini_buf_size; // 要超过h264 nalu 最小单元
    bool is_run ;
    bool bloop;
	void (*handler)(void* param, const uint8_t* nalu, size_t bytes);
	void *param;// handle param
}tyk_h264_parse;


int yk_h264_parse(tyk_h264_parse *parse);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
