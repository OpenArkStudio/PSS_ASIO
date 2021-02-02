#pragma once

#include "SessionInterface.h"
#include "ModuleInterfalce.h"
#include "serverconfigtype.h"
#include "ICommunicationService.h"
#include "ISessionService.h"
#include "IotoIo.h"

//根据线程的逻辑插件处理模块
//add by freeeyes

class CModuleLogic
{
public:
    CModuleLogic() = default;

    void init_logic(command_to_module_function command_to_module_function, uint16 work_thread_id);

    void add_session(uint32 connect_id, shared_ptr<ISession> session, const _ClientIPInfo& local_info, const _ClientIPInfo& romote_info);

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

    void init_work_thread_logic(int thread_count, config_logic_list& logic_list, ISessionService* session_service);

    void init_communication_service(ICommunicationInterface* communicate_service);

    void close();

    void add_thread_session(uint32 connect_id, shared_ptr<ISession> session, _ClientIPInfo& local_info, const _ClientIPInfo& romote_info);

    void delete_thread_session(uint32 connect_id, shared_ptr<ISession> session);

    void close_session_event(uint32 connect_id);

    int do_thread_module_logic(const uint32 connect_id, vector<CMessage_Packet>& message_list, shared_ptr<ISession> session);

    void send_io_message(uint32 connect_id, CMessage_Packet send_packet);

    bool connect_io_server(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type);

    void close_io_server(uint32 server_id);

    uint32 get_io_server_id(uint32 connect_id);

private:
    vector<shared_ptr<CModuleLogic>> thread_module_list_;
    CLoadModule load_module_;
    uint16      thread_count_ = 0;
    ICommunicationInterface* communicate_service_ = nullptr;
};

using App_WorkThreadLogic = PSS_singleton<CWorkThreadLogic>;
