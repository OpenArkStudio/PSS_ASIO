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
    virtual EM_CONNECT_IO_TYPE get_io_type() = 0; //获得当前IO状态
    virtual uint32 get_mark_id(uint32 connect_id) = 0; //获得当前链接被标记的ID
    virtual std::chrono::steady_clock::time_point& get_recv_time() = 0;   //得到接收数据时间
    virtual bool format_send_packet(uint32 connect_id, CMessage_Packet& message) = 0;  //格式化发送数据
};
