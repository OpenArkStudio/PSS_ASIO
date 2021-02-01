#pragma once

#include <iostream>
#include "define.h"
#include "asio.hpp"
#include "tms.hpp"

#include "SendBuffer.h"
#include "SessionBuffer.hpp"
#include "LoadPacketParse.h"
#include "ConnectCounter.h"
#include "ISession.h"
#include "ModuleLogic.h"

class CTTyServer : public std::enable_shared_from_this<CTTyServer>, public ISession
{
public:
    CTTyServer(shared_ptr<asio::serial_port> serial_port_param, uint32 packet_parse_id, uint32 max_recv_size, uint32 max_send_size);

    void start(uint32 server_id);

    void set_write_buffer(uint32 connect_id, const char* data, size_t length) final;

    void do_write(uint32 connect_id) final;

    void do_write_immediately(uint32 connect_id, const char* data, size_t length) final;

    void close(uint32 connect_id) final;

    void add_send_finish_size(uint32 connect_id, size_t send_length) final;

    EM_CONNECT_IO_TYPE get_io_type()  final;

    uint32 get_mark_id(uint32 connect_id) final;

private:
    void do_receive();

    void clear_write_buffer();

    shared_ptr<asio::serial_port> serial_port_param_= nullptr;
    uint32 connect_id_ = 0;
    uint32 server_id_ = 0;

    size_t recv_data_size_ = 0;
    size_t send_data_size_ = 0;

    CSessionBuffer session_recv_buffer_;
    CSessionBuffer session_send_buffer_;
    shared_ptr<_Packet_Parse_Info> packet_parse_interface_ = nullptr;

    EM_CONNECT_IO_TYPE io_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_TTY;
};

