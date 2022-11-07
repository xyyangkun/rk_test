/******************************************************************************
 *
 *       Filename:  librkmedia_vi_vo.h
 *
 *    Description: librkmedia vi vo test
 *
 *        Version:  1.0
 *        Created:  2022年10月31日 13时49分16秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef _LIBRKMEDIA_VI_VO_H_
#define _LIBRKMEDIA_VI_VO_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


// 初始化vi vo ,将 vi 输入的1080p信号旋转后输出mipi vo
int librkmedia_vi_vo_init();
// 释放库
int librkmedia_vi_vo_deinit();


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif//_LIBRKMEDIA_VI_VO_H_
