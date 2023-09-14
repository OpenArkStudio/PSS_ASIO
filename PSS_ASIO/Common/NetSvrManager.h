#pragma once

#include "TcpSSLServer.h"
#include "TcpServer.h"
#include "UdpServer.h"
#include "KcpServer.h"
#include "TtyServer.h"
#include "QueueService.h"
#include "serverconfig.h"
#include "CommunicationService.h"
#include "SessionService.h"
#include "IoContextPool.h"
#include "IoNetServer.h"
#include "IoListManager.h"

#if PSS_PLATFORM == PLATFORM_WIN
#include <tchar.h>
#endif

class CNetSvrManager : public CIo_List_Manager
{
public:
    CNetSvrManager();
    ~CNetSvrManager();
    void start_default_service();
    void close_all_service();

    void add_accept_net_io_event(string io_ip, io_port_type io_port, EM_CONNECT_IO_TYPE em_io_net_type, shared_ptr<CIo_Net_server> Io_Net_server) final;
    void del_accept_net_io_event(string io_ip, io_port_type io_port, EM_CONNECT_IO_TYPE em_io_net_type) final;

    void start_single_service(const CConfigNetIO& netio);
    void close_single_service(const CConfigNetIO& netio);

private:
    using map_tcp_server_list = unordered_map<string,shared_ptr<CIo_Net_server>>;
    map_tcp_server_list tcp_service_list_;

    using map_udp_server_list = unordered_map<string,shared_ptr<CIo_Net_server>>;
    map_udp_server_list udp_service_list_;

    using map_kcp_server_list = unordered_map<string,shared_ptr<CIo_Net_server>>;
    map_kcp_server_list kcp_service_list_;

    using map_tty_server_list = unordered_map<string,shared_ptr<CIo_Net_server>>;
    map_tty_server_list tty_service_list_;

#ifdef SSL_SUPPORT
    using hashipport2tcpsslserver = unordered_map<string,shared_ptr<CTcpSSLServer>>;
    hashipport2tcpsslserver tcp_ssl_service_map_;
#endif

    std::mutex list_mutex_;
};

using App_NetSvrManager = PSS_singleton<CNetSvrManager>;
