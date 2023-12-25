#pragma once

//规范化接口服务器指针
//add by freeeyes

#include "define.h"
#include "IoNetServer.h"

class CIo_List_Manager
{
public:
    virtual void add_accept_net_io_event(string io_ip, io_port_type io_port, EM_CONNECT_IO_TYPE em_io_net_type, shared_ptr<CIo_Net_server> Io_Net_server) = 0;
    virtual void del_accept_net_io_event(string io_ip, io_port_type io_port, EM_CONNECT_IO_TYPE em_io_net_type) = 0;
};