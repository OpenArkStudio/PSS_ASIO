#pragma once

//传递给插件的调用接口
//add by freeeyes

#include "ISessionService.h"
#include "ModuleLogic.h"

class CSessionService : public ISessionService
{
public:
    void send_io_message(uint32 connect_id, CMessage_Packet send_packet) final;
    bool connect_io_server(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type) final;
    void close_io_session(uint32 connect_id) final;
};

using App_SessionService = PSS_singleton<CSessionService>;
