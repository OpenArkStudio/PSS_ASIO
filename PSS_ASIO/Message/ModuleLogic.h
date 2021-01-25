#pragma once

#include "SessionInterfalce.h"
#include "ModuleInterfalce.h"
#include "serverconfigtype.h"

//根据线程的逻辑插件处理模块
//add by freeeyes

class CModuleLogic
{
public:
    CModuleLogic() = default;

    void init_logic(command_to_module_function command_to_module_function, uint16 work_thread_id);

    void add_session(uint32 connect_id, shared_ptr<ISession> session);

    shared_ptr<ISession> get_session_interface(uint32 connect_id);

    void delete_session_interface(uint32 connect_id);

    void close();

    int do_thread_module_logic(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet);

    uint16 get_work_thread_id();

private:
    CSessionInterface sessions_interface_;
    CModuleInterface modules_interface_;
    uint16 work_thread_id_ = 0;
};

class CWorkThreadLogic
{
public:
    CWorkThreadLogic() = default;

    void init_work_thread_logic(int thread_count, config_logic_list& logic_list);

    void close();

    void add_thread_session(uint32 connect_id, shared_ptr<ISession> session);

    void delete_thread_session(uint32 connect_id);

    void close_session_event(uint32 connect_id);

    int do_thread_module_logic(const uint32 connect_id, vector<CMessage_Packet>& message_list, shared_ptr<ISession> session);

private:
    vector<shared_ptr<CModuleLogic>> thread_module_list_;
    CLoadModule load_module_;
    uint16      thread_count_ = 0;
};

using App_WorkThreadLogic = PSS_singleton<CWorkThreadLogic>;
