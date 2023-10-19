#pragma once

#include "TcpSession.h"
#include "IoContextPool.h"
#include "IoListManager.h"

class CTcpServer : public std::enable_shared_from_this<CTcpServer>, public CIo_Net_server
{
public:
    CTcpServer(const CreateIoContextCallbackFunc& callback, const CConfigNetIO& config_io, CIo_List_Manager* io_list_manager);

    virtual ~CTcpServer() = default;

    void close() final;

    void start();

private:
    void do_accept();

    void send_accept_listen_fail(std::error_code ec);

    std::shared_ptr<tcp::acceptor> acceptor_;
    uint32 packet_parse_id_ = 0;
    uint32 max_recv_size_ = 0;
    CreateIoContextCallbackFunc callback_;
    
    string server_ip_;
    io_port_type server_port_;
    CIo_List_Manager* io_list_manager_ = nullptr;
};

