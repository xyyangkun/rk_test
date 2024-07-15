//
// Created by win10 on 2024/7/15.
//

#ifndef RK_TEST_OSEE_ERROR_H
#define RK_TEST_OSEE_ERROR_H

enum EB_EEROR
{
    EB_OK               = 0x00,	                      // 成功
    EB_FAILE            = 0x01,		                      // 命令失败
    EB_EMPTY_DATA       = 0x02,	                      // 空包
    EB_WRONG_HEAD       = 0x03,	                      // 包头错误
    EB_WRONG_LEN        = 0x04,	                      // 包长度错误

    EB_CHECKSUM_FAIL    =0X5,                 // CRC16检查错误
    EB_WRONG_AUTH       =0x06,	                      // 用户登陆名或密码错误

    EB_PARAM_INVALID    = 101,           // 参数错误, 指令无法识别 ERROR_PARAM_NULL  101
    EB_OPERATION_FAIL   = 0x0B,	          // 操作失败, 子卡执行出错
    EB_UNSUPPORT_CMD    = 0x0C,	          // 不支持的命令,  指令不支持
    EB_NOMEM            = 95,            // ERROR_MEM                               95
    EB_BUSY             = 0x07,   			      // 忙等待
    EB_TIMEOUT          =103 ,                /// 超时 ERROR_TIMEOUT                           103
    EB_DISCONNECT       =104,                   /// ERROR_DISCONNECT            104
    EB_NO_RETURN = 0Xffffffff,  // 告诉后续处理函数，不用进行返回操作

};
#endif //RK_TEST_OSEE_ERROR_H
