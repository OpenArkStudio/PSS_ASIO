#include "SessionService.h"

void CSessionService::send_io_message(uint32 connect_id, CMessage_Packet send_packet)
{
    App_WorkThreadLogic::instance()->send_io_message(connect_id, send_packet);
}

bool CSessionService::connect_io_server(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type)
{
    return App_WorkThreadLogic::instance()->connect_io_server(io_info, io_type);
}

void CSessionService::close_io_server(uint32 server_id)
{
    App_WorkThreadLogic::instance()->close_io_server(server_id);
}

