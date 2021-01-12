#pragma once

#include "TcpSession.h"

class CTcpServer
{
public:
    CTcpServer(asio::io_context& io_context, std::string server_ip, short port, uint32 packet_parse_id, uint32 max_buffer_size);

private:
    void do_accept();

    tcp::acceptor acceptor_;
    uint32 connect_clinet_id_ = 0;
    uint32 packet_parse_id_ = 0;
    uint32 max_buffer_size_ = 0;
};

