#pragma once

#include "define.h"

//��¶��������õĽӿ�
//add by freeeyes

class ISessionService
{
public:
    virtual void send_io_message(uint32 connect_id, CMessage_Packet send_packet) = 0;
    virtual bool connect_io_server(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type) = 0;
    virtual void close_io_session(uint32 connect_id) = 0;
    virtual bool add_session_io_mapping(_ClientIPInfo from_io, EM_CONNECT_IO_TYPE from_io_type, _ClientIPInfo to_io, EM_CONNECT_IO_TYPE to_io_type) = 0;
    virtual bool delete_session_io_mapping(_ClientIPInfo from_io, EM_CONNECT_IO_TYPE from_io_type) = 0;
};