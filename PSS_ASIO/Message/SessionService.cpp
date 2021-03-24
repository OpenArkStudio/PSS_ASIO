﻿#include "SessionService.h"

void CSessionService::send_io_message(uint32 connect_id, std::shared_ptr<CMessage_Packet> send_packet)
{
    App_WorkThreadLogic::instance()->send_io_message(connect_id, send_packet);
}

bool CSessionService::connect_io_server(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type)
{
    if (io_info.server_id == 0)
    {
        PSS_LOGGER_INFO("[CSessionService::connect_io_server]server id must over 0, connect fail.");
        return false;
    }

    return App_WorkThreadLogic::instance()->connect_io_server(io_info, io_type);
}

void CSessionService::close_io_session(uint32 connect_id)
{
    auto server_id = App_WorkThreadLogic::instance()->get_io_server_id(connect_id);
    if (server_id > 0)
    {
        App_WorkThreadLogic::instance()->close_io_server(server_id);
    }

    //关闭链接
    App_WorkThreadLogic::instance()->close_session_event(connect_id);

}

bool CSessionService::add_session_io_mapping(_ClientIPInfo from_io, EM_CONNECT_IO_TYPE from_io_type, _ClientIPInfo to_io, EM_CONNECT_IO_TYPE to_io_type)
{
    return App_WorkThreadLogic::instance()->add_session_io_mapping(from_io,
        from_io_type,
        to_io,
        to_io_type);
}

bool CSessionService::delete_session_io_mapping(_ClientIPInfo from_io, EM_CONNECT_IO_TYPE from_io_type)
{
    return App_WorkThreadLogic::instance()->delete_session_io_mapping(from_io,
        from_io_type);
}

bool CSessionService::send_frame_message(uint16 tag_thread_id, std::string message_tag, std::shared_ptr<CMessage_Packet> send_packet, CFrame_Message_Delay delay_timer)
{
    return App_WorkThreadLogic::instance()->send_frame_message(tag_thread_id,
        message_tag,
        send_packet,
        delay_timer);
}

bool CSessionService::run_work_thread_logic(uint16 tag_thread_id, CFrame_Message_Delay delay_timer, task_function func)
{
    return App_WorkThreadLogic::instance()->run_work_thread_logic(tag_thread_id, delay_timer, func);
}

bool CSessionService::create_frame_work_thread(uint32 thread_id)
{
    return App_WorkThreadLogic::instance()->create_frame_work_thread(thread_id);
}

bool CSessionService::close_frame_work_thread(uint32 thread_id)
{
    return App_WorkThreadLogic::instance()->close_frame_work_thread(thread_id);
}

bool CSessionService::delete_frame_message_timer(uint64 timer_id)
{
    return App_WorkThreadLogic::instance()->delete_frame_message_timer(timer_id);
}

uint16 CSessionService::get_io_work_thread_count()
{
    return App_WorkThreadLogic::instance()->get_io_work_thread_count();
}

uint16 CSessionService::get_plugin_work_thread_count()
{
    return App_WorkThreadLogic::instance()->get_plugin_work_thread_count();
}

