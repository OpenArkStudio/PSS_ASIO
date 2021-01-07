#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include "asio.hpp"
#include "tms.hpp"
#include "singleton.h"

#include "SessionBuffer.hpp"
#include "LoadPacketParse.h"

using asio::ip::tcp;

using App_tms = PSS_singleton<TMS>;

class CSendBuffer
{
public:
    string data_;
    std::size_t buffer_length_ = 0;

    void set(const char* _buffer, std::size_t _buffer_length)
    {
        data_.append(_buffer, _buffer_length);
        buffer_length_ = _buffer_length;
    }
};

class CTcpSession : public std::enable_shared_from_this<CTcpSession>
{
public:
    CTcpSession(tcp::socket socket);

    void open(uint32 connect_id, uint32 packet_parse_id);

    void Close();

private:
    void do_read();

    void do_write(CMessage_Packet send_packet);

    tcp::socket socket_;
    uint32 connect_id_ = 0;
    CSessionBuffer session_recv_buffer_;
    shared_ptr<_Packet_Parse_Info> packet_parse_interface_;

};

