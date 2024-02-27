#include "Iobridge.h"


bool CIoBridge::add_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& to_io, EM_CONNECT_IO_TYPE to_io_type, ENUM_IO_BRIDGE_TYPE bridge_type)
{
    iotoio_.add_session_io_mapping(from_io, from_io_type, to_io, to_io_type, bridge_type);
    return true;
}

bool CIoBridge::delete_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type)
{
    //查找对应的IO并关闭两端的状态
    iotoio_.delete_session_io_mapping(from_io, from_io_type);
    return true;
}

void CIoBridge::regedit_bridge_session_info(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id, std::shared_ptr<ISession> from_session)
{
    iotoio_.regedit_bridge_session_info(from_io, io_type, session_id, from_session);
}

void CIoBridge::unregedit_bridge_session_info(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id)
{
    iotoio_.unregedit_bridge_session_info(from_io, io_type, session_id);
}

