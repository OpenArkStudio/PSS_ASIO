#pragma once

#ifdef SSL_SUPPORT

#include "TcpSSLSession.h"
#include "IoContextPool.h"
#include "IoListManager.h"

class CTcpSSLServer : public std::enable_shared_from_this<CTcpSSLServer>, public CIo_Net_server
{
public:
    CTcpSSLServer(CreateIoContextCallbackFunc callback, 
        std::string server_ip, 
        io_port_type port,
        uint32 packet_parse_id, 
        uint32 max_recv_size, 
        std::string ssl_server_password,
        std::string ssl_server_pem_file,
        std::string ssl_server_dh_file,
        CIo_List_Manager* io_list_manager);

    void close() const;

    void start();

private:
    std::string get_password() const;

    void do_accept();

    void send_accept_listen_fail(std::error_code ec) const;

    std::string server_ip_ = "";
    io_port_type server_port_ = 0;

    std::shared_ptr<tcp::acceptor> acceptor_;
    asio::ssl::context context_;
    uint32 packet_parse_id_ = 0;
    uint32 max_recv_size_ = 0;
    std::string ssl_server_password_;
    std::string ssl_server_pem_file_;
    std::string ssl_server_dh_file_;
    CreateIoContextCallbackFunc callback_;
    CIo_List_Manager* io_list_manager_ = nullptr;
};

#endif
