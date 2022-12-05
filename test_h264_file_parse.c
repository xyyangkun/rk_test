/******************************************************************************
 *
 *       Filename:  test_h264_file_parse.c
 *
 *    Description:  test h264 file parse
 *    读出h264 nalu帧数据后，再写入文件
 *
 *        Version:  1.0
 *        Created:  2022年12月02日 14时38分02秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include "h264_file_parse.h"

typedef struct s_h264_dec
{
	FILE *fp_h264_out;
}t_h264_dec;



static void h264_handler(void* param, const uint8_t* nalu, size_t bytes)
{
	printf("===============>%p %lu> %02x %02x %02x %02x\n", param, bytes, nalu[0], nalu[1], nalu[2], nalu[3]);

	if(!param)return ;
	t_h264_dec *dec = (t_h264_dec *)param;

	// 数据数据
	if(dec->fp_h264_out)fwrite(nalu, bytes, 1, dec->fp_h264_out);

}

int main(int argc, char *argv[])
{
	if(argc == 1)
	{
		printf("useage:%s filepath\n", argv[0]);
		return -1;
	}


	tyk_h264_parse parse = {0};
	t_h264_dec dec;
	// 打开文件
	FILE* fp_h264_out= fopen("out.h264", "wb+");
	dec.fp_h264_out = fp_h264_out;

	// 参数
	strcpy(parse.file_name, argv[1]);
	parse.mini_buf_size= 1024*1024;
	parse.is_run = 1;
	parse.bloop = 0;
	parse.handler = h264_handler;
	parse.param = &dec;


	int ret = yk_h264_parse(&parse);
	if(ret != 0)
	{
		printf("ERROR in yk parse\n");
		exit(1);
	}


	// 关闭文件
	if(fp_h264_out){
		fclose(fp_h264_out);
	}
	return 0;
}
