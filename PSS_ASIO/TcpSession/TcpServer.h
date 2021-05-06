#pragma once

#include "TcpSession.h"

class CTcpServer
{
public:
    CTcpServer(asio::io_context& io_context, const std::string& server_ip, short port, uint32 packet_parse_id, uint32 max_recv_size);

    void close() const;

private:
    void do_accept();

    void send_accept_listen_fail(std::error_code ec) const;

    std::shared_ptr<tcp::acceptor> acceptor_;
    uint32 packet_parse_id_ = 0;
    uint32 max_recv_size_ = 0;
};

