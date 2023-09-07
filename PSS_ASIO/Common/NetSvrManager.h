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

#if PSS_PLATFORM == PLATFORM_WIN
#include <tchar.h>
#endif

class CNetSvrManager : private asio::noncopyable
{
public:
    CNetSvrManager();
    ~CNetSvrManager();
    void start_default_service();
    void close_all_service();

    void start_single_service(const CConfigNetIO& netio);
    void close_single_service(const CConfigNetIO& netio);

private:
    using hashipport2tcpserver = unordered_map<string,shared_ptr<CTcpServer>>;
    hashipport2tcpserver tcp_service_map_;

    using hashipport2udpserver = unordered_map<string,shared_ptr<CUdpServer>>;
    hashipport2udpserver udp_service_map_;

    using hashipport2kcpserver = unordered_map<string,shared_ptr<CKcpServer>>;
    hashipport2kcpserver kcp_service_map_;

    using hashttyname2tcpserver = unordered_map<string,shared_ptr<CTTyServer>>;
    hashttyname2tcpserver tty_service_map_;

#ifdef SSL_SUPPORT
    using hashipport2tcpsslserver = unordered_map<string,shared_ptr<CTcpSSLServer>>;
    hashipport2tcpsslserver tcp_ssl_service_map_;
#endif
};

using App_NetSvrManager = PSS_singleton<CNetSvrManager>;
