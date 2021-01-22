#pragma once

//服务主入口
//add by freeeyes

#include "TcpServer.h"
#include "UdpServer.h"
#include "TtyServer.h"
#include "CommunicationService.h"

#if PSS_PLATFORM == PLATFORM_WIN
#include <tchar.h>
#endif

class CServerService
{
public:
    bool init_servce();

    void close_service();

    void stop_service();

private:
    vector<shared_ptr<CTcpServer>> tcp_service_list_;
    vector<shared_ptr<CUdpServer>> udp_service_list_;
    vector<shared_ptr<CTTyServer>> tty_service_list_;

    asio::io_context io_context_;
};

using App_ServerService = PSS_singleton<CServerService>;
