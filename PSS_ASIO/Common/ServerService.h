#pragma once

//服务主入口
//add by freeeyes

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

class CServerService
{
public:
    bool init_servce(const std::string& pss_config_file_name = config_file_name);

    void close_service();

    void stop_service();

private:
    vector<shared_ptr<CTcpServer>> tcp_service_list_;
    vector<shared_ptr<CUdpServer>> udp_service_list_;
    vector<shared_ptr<CKcpServer>> kcp_service_list_;
    vector<shared_ptr<CTTyServer>> tty_service_list_;

#ifdef SSL_SUPPORT
    vector<shared_ptr<CTcpSSLServer>> tcp_ssl_service_list_;
#endif
};

using App_ServerService = PSS_singleton<CServerService>;
