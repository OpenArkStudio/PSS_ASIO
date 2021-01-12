#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include "asio.hpp"
#include "tms.hpp"

#include "SendBuffer.h"
#include "SessionBuffer.hpp"
#include "LoadPacketParse.h"

using asio::ip::tcp;

class CTcpSession : public std::enable_shared_from_this<CTcpSession>
{
public:
    CTcpSession(tcp::socket socket);

    void open(uint32 connect_id, uint32 packet_parse_id, uint32 buffer_size);

    void Close();

    void do_read();

    void do_write();

    void set_write_buffer(const char* data, size_t length);

    void clear_write_buffer();

    void add_send_finish_size(size_t send_length);

private:
    tcp::socket socket_;
    uint32 connect_id_ = 0;
    CSessionBuffer session_recv_buffer_;
    CSessionBuffer session_send_buffer_;
    shared_ptr<_Packet_Parse_Info> packet_parse_interface_;

    size_t recv_data_size_ = 0;
    size_t send_data_size_ = 0;
};

