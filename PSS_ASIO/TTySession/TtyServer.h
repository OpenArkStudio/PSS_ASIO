#pragma once

#include <iostream>
#include "define.h"
#include "asio.hpp"
#include "tms.hpp"

#include "SendBuffer.h"
#include "SessionBuffer.hpp"
#include "LoadPacketParse.h"
#include "ConnectCounter.h"

class CTTyServer : public std::enable_shared_from_this<CTTyServer>
{
public:
    CTTyServer(shared_ptr<asio::serial_port> serial_port_param, uint32 packet_parse_id, uint32 max_buffer_length);

private:
    void do_receive();

    void set_write_buffer(const char* data, size_t length);

    void clear_write_buffer();

    void do_write();

    shared_ptr<asio::serial_port> serial_port_param_;
    uint32 connect_client_id_ = 0;
    uint32 packet_parse_id_ = 0;

    size_t recv_data_size_ = 0;
    size_t send_data_size_ = 0;

    CSessionBuffer session_recv_buffer_;
    CSessionBuffer session_send_buffer_;
    shared_ptr<_Packet_Parse_Info> packet_parse_interface_ = nullptr;
};

