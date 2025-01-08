#pragma once

#include <string>
#include <vector>
#include "ISession.h"
#include "LoadModule.h"

//同步在执行逻辑模块
//add by freeeyes

class CSyncLogic
{
public:
    CSyncLogic(void) = default;

    void Close();

    void Init(const command_to_module_function& command_to_module_function);

    void do_sync_message_list(CMessage_Source& source, vector<std::shared_ptr<CMessage_Packet>>& message_list, std::shared_ptr<CMessage_Packet> send_packet);

    bool do_sync_message(const uint16 command_id, const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);

private:
    command_to_module_function command_to_session_function_;    //同步线程执行列表
};

using App_SyncLogic = PSS_singleton<CSyncLogic>;