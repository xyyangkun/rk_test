/******************************************************************************
 *
 *       Filename:  test_easymedia.cpp
 *
 *    Description:  测试rkmedia 中的easymedia库,  里面为c++封装的接口
 *    1. 编码dec 
 *       src/rkmpp/mpp_decoder.cc
 *       examples/unitTest/rkmpp/mpp_dec_test.cc
 *    2. 解码enc
 *       src/rkmpp/mpp_encoder.cc
 *       examples/unitTest/rkmpp/mpp_enc_test.cc
 *    3. MediaBuffer mb 模块，怎么构建， 怎么释放
 *       examples/unitTest/buffer/buffer_pool_test.cc
 *       include/easymedia/buffer.h 
 *       RK_MPI_MB_CreateImageBuffer  返回mb
 *
 *       
 *
 *        Version:  1.0
 *        Created:  2022年12月06日 13时33分28秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yangkun (yk)
 *          Email:  xyyangkun@163.com
 *        Company:  yangkun.com
 *
 *****************************************************************************/
#include <stdio.h>
#include "buffer.h"

namespace easymedia { 
static int __free_mppframecontext(void *p) {
	printf("=====> yk debug: mpp freame free:%p\n", p);	
	return 0;
}
int test_buff()
{
	auto mb = std::make_shared<ImageBuffer>();
	mb->SetUserData(&mb, __free_mppframecontext);

	return 0;
}
}

int main()
{
	easymedia::test_buff();

	return 0;
}
