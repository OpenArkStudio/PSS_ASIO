#pragma once

#include "define.h"

//所有IO接口的虚类
//add by freeeyes

//提供一个格式化发送数据的函数接口

class ISession
{
public:
    ISession() = default;
    virtual ~ISession() = default;
    
    virtual void set_io_bridge_connect_id(uint32 from_io_connect_id, uint32 to_io_connect_id) = 0;      //设置对应的桥接io的id
    virtual _ClientIPInfo get_remote_ip(uint32 connect_id) = 0;     //获得发送数据端的信息
    virtual void set_write_buffer(uint32 connect_id, const char* data, size_t length) = 0; //写入些缓冲
    virtual void do_write(uint32 connect_id) = 0;        //写入IO
    virtual void do_write_immediately(uint32 connect_id, const char* data, size_t length) = 0; //立刻写入IO端口
    virtual void close(uint32 connect_id) = 0;        //关闭IO端口
    virtual void add_send_finish_size(uint32 connect_id, size_t send_length) = 0;  //返回写入IO成功字节
    virtual EM_CONNECT_IO_TYPE get_io_type() = 0; //获得当前IO状态
    virtual uint32 get_mark_id(uint32 connect_id) = 0; //获得当前链接被标记的ID
    virtual std::chrono::steady_clock::time_point& get_recv_time(uint32 connect_id = 0) = 0;   //得到接收数据时间
    virtual bool format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message) = 0;  //格式化发送数据
    virtual bool is_need_send_format() = 0;    //是否需要格式化发送
    virtual bool is_connect() { return true; };  //当前链接状态是否是链接的
};
