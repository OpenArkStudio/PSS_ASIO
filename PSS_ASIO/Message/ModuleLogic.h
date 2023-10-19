#pragma once

#include "SessionInterface.h"
#include "ModuleInterfalce.h"
#include "serverconfigtype.h"
#include "ICommunicationService.h"
#include "ISessionService.h"
#include "IotoIo.h"
#include "CoreTimer.hpp"
#include "SessionBuffer.hpp"

//根据线程的逻辑插件处理模块
//add by freeeyes

//工作线程的执行状态
enum class ENUM_WORK_THREAD_STATE
{
    WORK_THREAD_INIT = 0,
    WORK_THREAD_BEGIN,
    WORK_THREAD_END,
};

//发送插件的json字符串格式
const std::string JSON_MODULE_THREAD_ID = R"({"thread id":)";
const std::string JSON_MODULE_COMMAND_ID = R"(, "command id":")";
const std::string JSON_MODULE_WORK_THREAD_TIMEOUT = R"(", "work_thread_timeout":)";
const std::string JSON_MODULE_END = R"("})";

//以消息模式处理逻辑代码
class CDelayPluginMessage
{
public:
    CDelayPluginMessage() = default;
    uint16 tag_thread_id_ = 0;
    std::string message_tag_ = "";
    std::shared_ptr<CMessage_Packet> send_packet_;
    CFrame_Message_Delay delay_timer_;
};

//以lambda模式处理代码
class CDelayPluginFunc
{
public:
    CDelayPluginFunc() = default;
    uint16 tag_thread_id_ = 0;
    CFrame_Message_Delay delay_timer_;
    task_function func_;
};

class CModuleLogic
{
public:
    CModuleLogic() = default;

    void init_logic(const command_to_module_function& command_to_module_function, uint16 work_thread_id);

    void add_session(uint32 connect_id, shared_ptr<ISession> session, const _ClientIPInfo& local_info, const _ClientIPInfo& romote_info);

    shared_ptr<ISession> get_session_interface(uint32 connect_id);

    void delete_session_interface(uint32 connect_id);

    void close();

    int do_thread_module_logic(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);

    uint16 get_work_thread_id() const;

    int get_work_thread_timeout() const;

    void check_session_io_timeout(uint32 connect_timeout);

    uint16 get_last_dispose_command_id() const;

    void each_session_id(const session_func& session_fn) const;

private:
    CSessionInterface sessions_interface_;
    CModuleInterface modules_interface_;
    uint16 work_thread_id_ = 0;
    uint16 last_dispose_command_id_ = 0;

    ENUM_WORK_THREAD_STATE work_thread_state_ = ENUM_WORK_THREAD_STATE::WORK_THREAD_INIT;
    std::chrono::steady_clock::time_point work_thread_run_time_ = std::chrono::steady_clock::now();
};

class CWorkThreadLogic 
{
public:
    CWorkThreadLogic() = default;

    void init_work_thread_logic(int thread_count, uint16 timeout_seconds, uint32 connect_timeout, uint16 io_send_time_check, const config_logic_list& logic_list, ISessionService* session_service);

    void init_communication_service(ICommunicationInterface* communicate_service);

    void close();

    void do_work_thread_frame_events(uint16 command_id, uint32 mark_id, const std::string& remote_ip, uint16 remote_port, EM_CONNECT_IO_TYPE io_type);

    void add_frame_events(uint16 command_id, uint32 mark_id, const std::string& remote_ip, uint16 remote_port, EM_CONNECT_IO_TYPE io_type);

    void add_thread_session(uint32 connect_id, shared_ptr<ISession> session, const _ClientIPInfo& local_info, const _ClientIPInfo& romote_info);

    void delete_thread_session(uint32 connect_id, shared_ptr<ISession> session);

    void close_session_event(uint32 connect_id);

    int assignation_thread_module_logic(const uint32 connect_id, const vector<shared_ptr<CMessage_Packet>>& message_list, shared_ptr<ISession> session);

    int assignation_thread_module_logic_iotoio_error(const uint32 connect_id, const vector<shared_ptr<CMessage_Packet>>& message_list, shared_ptr<ISession> session);

    int assignation_thread_module_logic_with_events(const uint32 connect_id, const vector<shared_ptr<CMessage_Packet>>& message_list, shared_ptr<ISession> session);

    void do_work_thread_module_logic(shared_ptr<ISession> session, const uint32 connect_id, const vector<shared_ptr<CMessage_Packet>>& message_list, shared_ptr<CModuleLogic> module_logic) const;

    void do_io_message_delivery(uint32 connect_id, std::shared_ptr<CMessage_Packet> send_packet, shared_ptr<CModuleLogic> module_logic);

    void send_io_message(uint32 connect_id, std::shared_ptr<CMessage_Packet> send_packet);

    bool send_io_bridge_message(uint32 io_bridge_connect_id, std::shared_ptr<CMessage_Packet> send_packet);

    bool connect_io_server(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type);

    void close_io_server(uint32 server_id);

    uint32 get_io_server_id(uint32 connect_id);

    void run_check_task(uint32 timeout_seconds);

    bool send_frame_message(uint16 tag_thread_id, const std::string& message_tag, std::shared_ptr<CMessage_Packet> send_packet, CFrame_Message_Delay delay_timer);

    bool do_frame_message(uint16 tag_thread_id, const std::string& message_tag, std::shared_ptr<CMessage_Packet> send_packet, CFrame_Message_Delay delay_timer);

    bool run_work_thread_logic(uint16 tag_thread_id, CFrame_Message_Delay delay_timer, const task_function& func);

    bool do_work_thread_logic(uint16 tag_thread_id, CFrame_Message_Delay delay_timer, const task_function& func);

    void do_plugin_thread_module_logic(shared_ptr<CModuleLogic> module_logic, const std::string& message_tag, std::shared_ptr<CMessage_Packet> recv_packet) const;

    bool create_frame_work_thread(uint32 thread_id);

    bool close_frame_work_thread(uint32 thread_id);

    bool delete_frame_message_timer(uint64 timer_id);

    uint16 get_io_work_thread_count() const;

    uint16 get_plugin_work_thread_count() const;

    int module_run(const std::string& module_name, std::shared_ptr<CMessage_Packet> send_packet, std::shared_ptr<CMessage_Packet> return_packet);

    uint32 get_curr_thread_logic_id() const;

    void send_io_buffer() const;

    uint32 get_connect_id(uint32 server_id) const;

    void regedit_bridge_session_id(uint32 connect_id) const;

    bool set_io_bridge_connect_id(uint32 from_io_connect_id, uint32 to_io_connect_id);

    int do_io_bridge_data(uint32 connect_id, uint32 io_bridge_connect_id_, CSessionBuffer& session_recv_buffer, std::size_t length, shared_ptr<ISession> session);

private:
    void send_io_buffer_to_session(uint32 connect_id, std::shared_ptr<ISession> session, std::shared_ptr<CMessage_Packet> format_packet) const;
    void do_work_thread_timeout(uint16 work_thread_id, uint16 last_dispose_command_id, int work_thread_timeout);
    void send_io_bridge_message_fail(uint32 connect_id, std::shared_ptr<CMessage_Packet> bridge_packet, shared_ptr<ISession> session);

    using hashmappluginworkthread = unordered_map<uint32, shared_ptr<CModuleLogic>>;
    using hashmaplogictimer = unordered_map<uint64, brynet::Timer::WeakPtr>;
    vector<shared_ptr<CModuleLogic>> thread_module_list_;
    CLoadModule load_module_;
    uint16      thread_count_ = 0;
    ICommunicationInterface* communicate_service_ = nullptr;
    bool        module_init_finish_ = false;
    vector<uint32> plugin_work_thread_buffer_list_;
    vector<CDelayPluginMessage> plugin_work_thread_buffer_message_list_;
    vector<std::shared_ptr<CDelayPluginFunc>> plugin_work_thread_buffer_Func_list_;
    hashmappluginworkthread plugin_work_thread_list_;
    hashmaplogictimer plgin_timer_list_;
    std::recursive_mutex plugin_timer_mutex_;
    uint32 connect_timeout_ = 0;
    uint16 io_send_time_check_ = 0;
};

using App_WorkThreadLogic = PSS_singleton<CWorkThreadLogic>;
