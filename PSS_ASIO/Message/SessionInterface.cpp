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
    PSS_LOGGER_INFO("[CSessionInterface::delete_session_interface]connect_id={0}.", connect_id);
    sessions_list_.erase(connect_id);
}

void CSessionInterface::check_session_io_timeout(uint32 connect_timeout, vector<CSessionIO_Cancel>& session_list) const
{
    auto check_connect_time_ = std::chrono::steady_clock::now();

    for (const auto& session_io : sessions_list_)
    {
        //目前只检查tcp
        if (session_io.second.session_->get_io_type() == EM_CONNECT_IO_TYPE::CONNECT_IO_TCP)
        {
            std::chrono::duration<double, std::ratio<1, 1>> elapsed = check_connect_time_ - session_io.second.session_->get_recv_time();
            if (elapsed.count() >= connect_timeout)
            {
                PSS_LOGGER_INFO("[CSessionInterface::check_session_io_timeout]elapsed={0}.", elapsed.count());
                
                CSessionIO_Cancel session_cancel;
                session_cancel.session_id_ = session_io.first;
                session_cancel.session_ = session_io.second.session_;
                
                //添加删除列表
                session_list.emplace_back(session_cancel);
            }
        }
    }

}

std::vector<uint32> CSessionInterface::get_all_session_id() const
{
    vector<uint32> session_id_list;

    for (const auto& session_info : sessions_list_)
    {
        session_id_list.push_back(session_info.first);
    }

    return session_id_list;
}
