/******************************************************************************
 *
 *       Filename:  mb_get.h
 *
 *    Description: 将mpp buff 转换为rkmedia buff
 *
 *        Version:  1.0
 *        Created:  2022年12月06日 17时50分44秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#ifndef _MB_GET_H_
#define _MB_GET_H_

#include "rkmedia_api.h"

#ifdef __cplusplus
extern "C" {
#endif

MEDIA_BUFFER RK_MPI_MB_from_mpp(MppFrame mppframe);
int RK_MPI_MB_release(MEDIA_BUFFER mb);

#ifdef __cplusplus
}
#endif

#endif//_MB_GET_H_
