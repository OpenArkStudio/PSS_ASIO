#pragma once

#include "define.h"

//暴露给插件调用的接口
//add by freeeyes

class ISessionService
{
public:
    virtual ~ISessionService() {};
    virtual void get_server_listen_info(std::vector<CConfigNetIO>& io_list, EM_CONNECT_IO_TYPE io_type) = 0;
    virtual void send_io_message(uint32 connect_id, std::shared_ptr<CMessage_Packet> send_packet) = 0;
    virtual bool connect_io_server(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type) = 0;
    virtual void close_io_session(uint32 connect_id) = 0;
    virtual bool add_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& to_io, EM_CONNECT_IO_TYPE to_io_type) = 0;
    virtual bool delete_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type) = 0;
    virtual bool send_frame_message(uint16 tag_thread_id, const std::string& message_tag, std::shared_ptr<CMessage_Packet> send_packet, CFrame_Message_Delay delay_timer) = 0;
    virtual bool run_work_thread_logic(uint16 tag_thread_id, CFrame_Message_Delay delay_timer, const task_function& func) = 0;
    virtual bool delete_frame_message_timer(uint64 timer_id) = 0;
    virtual bool create_frame_work_thread(uint32 thread_id) = 0;
    virtual bool close_frame_work_thread(uint32 thread_id) = 0;
    virtual uint16 get_io_work_thread_count() = 0;
    virtual uint16 get_plugin_work_thread_count() = 0;
    virtual int module_run(const std::string& module_name, std::shared_ptr<CMessage_Packet> send_packet, std::shared_ptr<CMessage_Packet> return_packet) = 0;
    virtual uint32 get_curr_thread_logic_id() = 0;
};
