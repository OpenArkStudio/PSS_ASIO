#pragma once

//服务主入口
//add by freeeyes
#include "QueueService.h"
#include "serverconfig.h"
#include "CommunicationService.h"
#include "SessionService.h"
#include "NetSvrManager.h"

#if PSS_PLATFORM == PLATFORM_WIN
#include <tchar.h>
#endif

class CServerService
{
public:
    bool init_service(const std::string& pss_config_file_name = config_file_name);
    void close_service();
    void stop_service();
};

using App_ServerService = PSS_singleton<CServerService>;
