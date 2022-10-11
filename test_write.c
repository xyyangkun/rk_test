/******************************************************************************
 *
 *       Filename:  test_write.c
 *
 *    Description:  测试写入
 *
 *        Version:  1.0
 *        Created:  2022年10月11日 13时17分39秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <stdio.h>
#include <sys/vfs.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#if 0
#define _LINE_LENGTH 300
int get_sd_avail1(unsigned long long *bfree, unsigned long long *bavail)
{
    char line[_LINE_LENGTH];
    FILE *fp = NULL;

	// 检查剩余空间
	const char *cmd = "df -k|grep sdcard|awk '{print $4}'";
	fp = popen(cmd, "r");
	if(NULL != fp)
	{
		memset(line, 0, sizeof(line));
		while (fgets(line, _LINE_LENGTH, fp) != NULL)
		{
			//printf("read df line=%s ", line);
		}
		int num=atoi(line);
		printf("========================> get df num:%d\n", num);

		// 小于200M 空间时停止录像
		if(num < 200*1024)
		{
			printf("will stop record!!\n");
			VideoSel *sel = reinterpret_cast<VideoSel *>(handle);
			if(sel){
				sel->record_stop();
			}
		}

		pclose(fp);
		fp = NULL;
	}

	return 0;
}
#endif
int get_sd_avail(unsigned long long *bfree, unsigned long long *bavail)
{
	struct statfs diskInfo = {0};
	int ret;
	// ret = statfs("/dev/mmcblk1",&diskInfo);
	ret = statfs("/mnt/sdcard/",&diskInfo);
	//ret = statfs("/dev/sda1",&diskInfo);
	if(ret != 0)
	{
		printf("failed to get,ret=%d\n", ret);
		return -1;
	}

	unsigned long long blocksize = diskInfo.f_bsize;// 每个block里面包含的字节数

	/*
	unsigned long long totalsize = blocksize*diskInfo.f_blocks;//总的字节数
	char totalsize_GB[10]={0};
	printf("TOTAL_SIZE == %llu KB  %llu MB  %llu GB\n",totalsize>>10,totalsize>>20,totalsize>>30); // 分别换成KB,MB,GB为单位
	sprintf(totalsize_GB,"%.2f",(float)(totalsize>>20)/1024);
	printf("totalsize_GB=%s\n",totalsize_GB);
	*/

	printf("avail=%ld   %ld\n", blocksize*diskInfo.f_bfree, blocksize*diskInfo.f_bavail);
	
	*bfree = blocksize*diskInfo.f_bfree;
	*bavail = blocksize*diskInfo.f_bavail;
	system("df /dev/mmcblk1");
	return 0;
}

int main()
{
	int size = 1024*1024;
	char *p = (char *)malloc(size);
	memset(p, 0xff, size);
	unsigned long long bfree, bavail;
	unsigned long long bfree1, bavail1;

	FILE *fp = fopen("/mnt/sdcard/a.txt", "wb");

	for(int i=0; i<100; i++)
	{
		// 写之前统计空间占用
		get_sd_avail(&bfree, &bavail);

		fwrite(p, size, 1, fp);
		int  fd = fileno(fp);
		fsync(fd);

		// 写之后统计空间占用
		bfree1 = bfree; bavail1 = bavail;
		get_sd_avail(&bfree, &bavail);
		if(bfree != bavail)
		{
			printf("ERROR !, bfree=%d, bavail=%d", bfree, bavail);
			exit(1);
		}

		int s = bavail1 - bavail;
		if(s != size)
		{
			printf("ERROR in size !, bfree=%d, bavail=%d diff = %d\n", bfree, bavail, s);
			exit(1);
		}

		printf("index =============> %d\n\n\n", i);
	}

	free(p);
	fclose(fp);
	return 0;
}
