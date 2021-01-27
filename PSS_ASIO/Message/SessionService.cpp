#include "SessionService.h"

void CSessionService::send_io_message(uint32 connect_id, CMessage_Packet send_packet)
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

    //¹Ø±ÕÁ´½Ó
    App_WorkThreadLogic::instance()->close_session_event(connect_id);

}

