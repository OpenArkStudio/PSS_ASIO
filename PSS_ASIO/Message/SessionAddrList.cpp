#include "SessionAddrList.h"

std::string CSessionAddrList::set_addr_key(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type)
{
    std::string from_io_key;

    if (io_type == EM_CONNECT_IO_TYPE::CONNECT_IO_TCP)
    {
        from_io_key = fmt::format("{0}:{1} TCP", from_io.m_strClientIP, from_io.m_u2Port);
    }
    else if (io_type == EM_CONNECT_IO_TYPE::CONNECT_IO_UDP)
    {
        from_io_key = fmt::format("{0}:{1} UDP", from_io.m_strClientIP, from_io.m_u2Port);
    }
    else
    {
        from_io_key = fmt::format("{0}:{1} TTY", from_io.m_strClientIP, from_io.m_u2Port);
    }

    return from_io_key;
}

void CSessionAddrList::add_session_addr(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id, std::shared_ptr<ISession> from_session)
{
    CSessionAddr_Info session_addr_info;

    session_addr_info.session_id = session_id;
    session_addr_info.session_ = from_session;

    auto session_key = set_addr_key(from_io, io_type);

    session_addr_list_[session_key] = session_addr_info;
}

void CSessionAddrList::del_session_addr(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type)
{
    auto session_key = set_addr_key(from_io, io_type);

    session_addr_list_.erase(session_key);
}

CSessionAddr_Info CSessionAddrList::get_session_addr_info(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type)
{
    CSessionAddr_Info session_addr_info;

    auto f = session_addr_list_.find(set_addr_key(from_io, io_type));
    if (f != session_addr_list_.end())
    {
        session_addr_info.session_ = f->second.session_;
        session_addr_info.session_id = f->second.session_id;
    }

    return session_addr_info;
}
