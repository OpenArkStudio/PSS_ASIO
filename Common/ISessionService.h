#pragma once

#include "define.h"
#include "shm_queue/shm_common.hpp"

//同步调用插件接口
using plugin_api_logic = std::function<std::string(std::string)>;

//暴露给插件调用的接口
//add by freeeyes

//逻辑调用框架接口信息
class ISessionService
{
public:
    virtual ~ISessionService() {};
    virtual void get_server_listen_info(std::vector<CConfigNetIO>& io_list, EM_CONNECT_IO_TYPE io_type) = 0;
    virtual void send_io_message(uint32 connect_id, std::shared_ptr<CMessage_Packet> send_packet) = 0;
    virtual bool connect_io_server(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type) = 0;
    virtual void close_io_session(uint32 connect_id) = 0;
    virtual bool send_frame_message(uint16 tag_thread_id, const std::string& message_tag, std::shared_ptr<CMessage_Packet> send_packet, CFrame_Message_Delay delay_timer) = 0;
    virtual bool run_work_thread_logic(uint16 tag_thread_id, CFrame_Message_Delay delay_timer, const task_function& func) = 0;
    virtual bool delete_frame_message_timer(uint64 timer_id) = 0;
    virtual bool create_frame_work_thread(uint32 thread_id) = 0;
    virtual bool close_frame_work_thread(uint32 thread_id) = 0;
    virtual uint16 get_io_work_thread_count() = 0;
    virtual uint16 get_plugin_work_thread_count() = 0;
    virtual int module_run(const std::string& module_name, std::shared_ptr<CMessage_Packet> send_packet, std::shared_ptr<CMessage_Packet> return_packet) = 0;
    virtual uint32 get_curr_thread_logic_id() = 0;
    virtual bool add_plugin_api(const std::string& api_name, const plugin_api_logic& func) = 0;
    virtual std::string do_plugin_api(const std::string& api_name, const std::string& api_func_param) = 0;
    virtual bool add_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& to_io, EM_CONNECT_IO_TYPE to_io_type, ENUM_IO_BRIDGE_TYPE bridge_type = ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH) = 0;
    virtual bool delete_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type) = 0;
    //share memory queue list API
    virtual bool create_queue(shm_queue::shm_key key, size_t message_size = shm_queue_list_size, int message_count = shm_queue_list_count) = 0;
    virtual bool close(shm_queue::shm_key key) = 0;
    virtual bool send_queue_message(shm_queue::shm_key key, const char* message_text, size_t len) = 0;
    virtual bool set_close_function(shm_queue::shm_key key, const shm_queue::queue_close_func& close_func) = 0;
    virtual bool set_error_function(shm_queue::shm_key key, const shm_queue::queue_error_func& error_func) = 0;
    virtual bool set_recv_function(shm_queue::shm_key key, const shm_queue::queue_recv_message_func& fn_logic) = 0;

    virtual uint32 get_connect_id(uint32 server_id) = 0;

    virtual void regedit_bridge_session_id(uint32 connect_id = 0) = 0;
};
