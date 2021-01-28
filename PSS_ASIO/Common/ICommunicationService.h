#pragma once

#include "define.h"
#include "asio.hpp"

//服务器间链接接口虚类
//add by freeyes

class ICommunicationInterface
{
public:
    virtual void init_communication_service(asio::io_context* io_service_context, uint16 timeout_seconds) = 0;

    virtual bool add_connect(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type) = 0;

    virtual void set_connect_id(uint32 server_id, uint32 connect_id) = 0;

    virtual void reset_connect(uint32 server_id) = 0;

    virtual void close_connect(uint32 server_id) = 0;

    virtual bool is_exist(uint32 server_id) = 0;

    virtual void close() = 0;

    virtual uint32 get_server_id(uint32 connect_id) = 0;
};
