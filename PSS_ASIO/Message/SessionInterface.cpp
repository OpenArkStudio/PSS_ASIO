#include "SessionInterface.h"

void CSessionInterface::add_session_interface(uint32 connect_id, shared_ptr<ISession> session, const _ClientIPInfo& local_info, const _ClientIPInfo& romote_info)
{
    auto f = sessions_list_.find(connect_id);
    if (f == sessions_list_.end())
    {
        CSessionIOInfo session_info;
        session_info.local_info_ = local_info;
        session_info.romote_info_ = romote_info;
        session_info.session_ = session;
        sessions_list_[connect_id] = session_info;
    }
}

shared_ptr<ISession> CSessionInterface::get_session_interface(uint32 connect_id)
{
    auto f = sessions_list_.find(connect_id);
    if (f != sessions_list_.end())
    {
        return f->second.session_;
    }
    else
    {
        return nullptr;
    }
}

_ClientIPInfo CSessionInterface::get_session_local_ip(uint32 connect_id)
{
    _ClientIPInfo local_info;
    auto f = sessions_list_.find(connect_id);
    if (f != sessions_list_.end())
    {
        return f->second.local_info_;
    }
    else
    {
        return local_info;
    }
}

_ClientIPInfo CSessionInterface::get_session_remote_ip(uint32 connect_id)
{
    _ClientIPInfo remote_info;
    auto f = sessions_list_.find(connect_id);
    if (f != sessions_list_.end())
    {
        return f->second.romote_info_;
    }
    else
    {
        return remote_info;
    }
}

void CSessionInterface::delete_session_interface(uint32 connect_id)
{
    sessions_list_.erase(connect_id);
}

void CSessionInterface::close()
{
    sessions_list_.clear();
}
