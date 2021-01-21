#pragma once

#include "define.h"

//所有IO接口的虚类
//add by freeeyes

class ISession
{
public:
    ISession() = default;
    virtual ~ISession() = default;

    virtual void set_write_buffer(uint32 connect_id, const char* data, size_t length) = 0; //写入些缓冲
    virtual void do_write(uint32 connect_id) = 0;        //写入IO
    virtual void do_write_immediately(uint32 connect_id, const char* data, size_t length) = 0; //立刻写入IO端口
    virtual void close(uint32 connect_id) = 0;        //关闭IO端口
    virtual void add_send_finish_size(uint32 connect_id, size_t send_length) = 0;  //返回写入IO成功字节
};
