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
    PSS_LOGGER_DEBUG("[CSessionInterface::delete_session_interface]connect_id={0} OK.", connect_id);
}

void CSessionInterface::check_session_io_timeout(uint32 connect_timeout, vector<CSessionIO_Cancel>& session_list)
{
    auto check_connect_time = std::chrono::steady_clock::now();
    if (!is_need_check_session(check_connect_time, connect_timeout))
    {
        return;
    }

    PSS_LOGGER_DEBUG("[CSessionInterface::check_session_io_timeout]****sessions_list_.size={}, connect_timeout={}.", sessions_list_.size(), connect_timeout);
    for (const auto& session_io : sessions_list_)
    {
        //检查tcp
        if (session_io.second.session_->get_io_type() == EM_CONNECT_IO_TYPE::CONNECT_IO_TCP)
        {
            std::chrono::duration<double, std::ratio<1, 1>> elapsed = check_connect_time_ - session_io.second.session_->get_recv_time();
            PSS_LOGGER_DEBUG("[CSessionInterface::check_session_io_timeout]****tcp sessions id={}, elapsed={}.", session_io.second.session_->get_connect_id(), elapsed.count());
            if (elapsed.count() >= connect_timeout)
            {
                PSS_LOGGER_DEBUG("[CSessionInterface::check_session_io_timeout]connectid={},elapsed={}.",session_io.first, elapsed.count());
                
                CSessionIO_Cancel session_cancel;
                session_cancel.session_id_ = session_io.first;
                session_cancel.session_ = session_io.second.session_;
                
                //添加删除列表
                session_list.emplace_back(session_cancel);
            }
        }
        else if (session_io.second.session_->get_io_type() == EM_CONNECT_IO_TYPE::CONNECT_IO_UDP)
        {
            //检查UDP
            std::chrono::duration<double, std::ratio<1, 1>> elapsed = check_connect_time_ - session_io.second.session_->get_recv_time(session_io.first);
            PSS_LOGGER_DEBUG("[CSessionInterface::check_session_io_timeout]****udp sessions id={}, elapsed={}.", session_io.second.session_->get_connect_id(), elapsed.count());
            if (elapsed.count() >= connect_timeout)
            {
                PSS_LOGGER_DEBUG("[CSessionInterface::check_session_io_timeout]connectid={},elapsed={}.",session_io.first, elapsed.count());
                
                CSessionIO_Cancel session_cancel;
                session_cancel.session_id_ = session_io.first;
                session_cancel.session_ = session_io.second.session_;
                
                //添加删除列表
                session_list.emplace_back(session_cancel);
            }
        }
    }

}

bool CSessionInterface::is_need_check_session(const std::chrono::steady_clock::time_point& check_connect_time, const uint32& connect_timeout)
{
    std::chrono::duration<double, std::ratio<1, 1>> elapsed = check_connect_time - check_connect_time_;
    if (elapsed.count() >= connect_timeout)
    {
        check_connect_time_ = check_connect_time;
        return true;
    }
    else
    {
        return false;
    }
}

void CSessionInterface::start_check()
{
    check_connect_time_ = std::chrono::steady_clock::now();
}

