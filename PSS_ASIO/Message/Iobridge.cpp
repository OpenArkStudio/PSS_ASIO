#include "Iobridge.h"

bool CIoBridge::add_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& to_io, EM_CONNECT_IO_TYPE to_io_type, ENUM_IO_BRIDGE_TYPE bridge_type)
{
    auto ret = iotoio_.add_session_io_mapping(from_io, from_io_type, to_io, to_io_type, bridge_type);

    if (true == ret)
    {
        //判断当前链接是否已存在,如果存在，则添加桥接信息(from)
        auto from_addr_info = sesion_addr_list_.get_session_addr_info(from_io, from_io_type);
        if (from_addr_info.session_id > 0 && from_addr_info.session_ != nullptr)
        {
            regedit_bridge_session_info(from_io, from_io_type, from_addr_info.session_id, from_addr_info.session_);
        }

        //判断当前链接是否已存在,如果存在，则添加桥接信息(to)
        auto to_addr_info = sesion_addr_list_.get_session_addr_info(to_io, to_io_type);
        if (to_addr_info.session_id > 0 && to_addr_info.session_ != nullptr)
        {
            regedit_bridge_session_info(to_io, to_io_type, to_addr_info.session_id, to_addr_info.session_);
        }
    }

    return ret;
}

bool CIoBridge::delete_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type)
{
    //查找对应的IO并关闭两端的状态
    iotoio_.delete_session_io_mapping(from_io, from_io_type);
    return true;
}

void CIoBridge::regedit_bridge_session_info(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id, std::shared_ptr<ISession> from_session)
{
    //添加session注册信息
    sesion_addr_list_.add_session_addr(from_io, io_type, session_id, from_session);

    iotoio_.regedit_bridge_session_info(from_io, io_type, session_id, from_session);
}

void CIoBridge::unregedit_bridge_session_info(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id)
{
    //删除session注册信息
    sesion_addr_list_.del_session_addr(from_io, io_type);

    iotoio_.unregedit_bridge_session_info(from_io, io_type, session_id);
}

