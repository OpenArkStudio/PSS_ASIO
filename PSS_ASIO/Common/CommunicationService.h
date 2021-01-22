#pragma once

#include "define.h"
#include "TcpClientSession.h"
#include "UdpClientSession.h"
#include "CoreTimer.hpp"

//管理服务器间链接，自动重连的类
//add by freeeyes

class CCommunicationService
{
public:
    CCommunicationService() = default;

    void init_communication_service(asio::io_context& io_service_context);

    void run_check_task();

private:
    using communication_list = unordered_map<uint32, shared_ptr<ISession>>;
    communication_list communication_list_;
    std::mutex thread_lock_;
};

using App_CommunicationService = PSS_singleton<CCommunicationService>;
