#pragma once

//处理消息和投递的类
//add by freeeyes

#include "define.h"
#include "ISession.h"

//记录链接IP地址
class CSessionIOInfo
{
public:
    _ClientIPInfo local_info_;
    _ClientIPInfo romote_info_;
    shared_ptr<ISession> session_;
};

class CSessionIO_Cancel
{
public:
    uint32 session_id_ = 0;
    shared_ptr<ISession> session_;
};

using session_func = std::function<void(uint32)>;

class CSessionInterface
{
public:
    CSessionInterface() = default;

    void add_session_interface(uint32 connect_id, shared_ptr<ISession> session, const _ClientIPInfo& local_info, const _ClientIPInfo& romote_info);

    shared_ptr<ISession> get_session_interface(uint32 connect_id);

    _ClientIPInfo get_session_local_ip(uint32 connect_id);

    _ClientIPInfo get_session_remote_ip(uint32 connect_id);

    void delete_session_interface(uint32 connect_id);

    void check_session_io_timeout(uint32 connect_timeout, vector<CSessionIO_Cancel>& session_list);

    bool is_need_check_session(std::chrono::steady_clock::time_point& check_connect_time, uint32& connect_timeout);

    void start_check();

    template<class session_function>
    void each_session_id(const session_function& session_fn) const
    {
        for (const auto& session_info : sessions_list_)
        {
            session_fn(session_info.first);
        }
    };
private:
    using hashmapsessions = unordered_map<uint32, CSessionIOInfo>;
    hashmapsessions sessions_list_;
    std::chrono::steady_clock::time_point check_connect_time_;
};