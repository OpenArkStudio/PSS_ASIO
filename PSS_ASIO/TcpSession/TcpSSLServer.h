#pragma once

#ifdef SSL_SUPPORT

#include "TcpSSLSession.h"

class CTcpSSLServer
{
public:
    CTcpSSLServer(asio::io_context& io_context, 
        std::string server_ip, 
        uint16 port, 
        uint32 packet_parse_id, 
        uint32 max_recv_size, 
        std::string ssl_server_password,
        std::string ssl_server_pem_file,
        std::string ssl_server_dh_file);

    void close() const;

private:
    std::string get_password() const;

    void do_accept();

    void send_accept_listen_fail(std::error_code ec) const;

    std::shared_ptr<tcp::acceptor> acceptor_;
    asio::ssl::context context_;
    uint32 packet_parse_id_ = 0;
    uint32 max_recv_size_ = 0;
    std::string ssl_server_password_;
    std::string ssl_server_pem_file_;
    std::string ssl_server_dh_file_;
    asio::io_context* io_context_ = nullptr;
};

#endif
