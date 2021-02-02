#include "IotoIo.h"

bool CIotoIo::add_session_io_mapping(_ClientIPInfo from_io, EM_CONNECT_IO_TYPE from_io_type, _ClientIPInfo to_io, EM_CONNECT_IO_TYPE to_io_type)
{
    for (auto Connect_Info : io_2_io_list_)
    {
        if (true == compare_connect_io(from_io, from_io_type, Connect_Info.from_io_, Connect_Info.from_io_type_))
        {
            //点对点链接已存在，不能添加
            PSS_LOGGER_DEBUG("[CIotoIo::add_session_io_mapping]from_ioio={}:{} is exist.", from_io.m_strClientIP, from_io.m_u2Port);
            return false;
        }
    }

    //添加新的映射
    CIo_Connect_Info connect_info;
    connect_info.from_io_ = from_io;
    connect_info.from_io_type_ = from_io_type;
    connect_info.to_io_ = to_io;
    connect_info.to_io_type_ = to_io_type;

    io_2_io_list_.emplace_back(connect_info);

    //查找当前链接是否存在了
    auto from_session_id = get_regedit_session_id(from_io, from_io_type);
    auto to_session_id = get_regedit_session_id(to_io, to_io_type);

    if (from_session_id > 0 && to_session_id > 0)
    {
        //建立连接关系
        CIo_Session_to_Session s_2_s;
        s_2_s.from_session_id_ = from_session_id;
        s_2_s.to_session_id_ = to_session_id;
        session_to_session_list_.emplace_back(s_2_s);
    }

    return true;
}

void CIotoIo::regedit_session_id(_ClientIPInfo from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id)
{
    auto from_io_key = get_connect_list_key(from_io, io_type);
    connect_list_[from_io_key] = session_id;

    //寻找链接是否已经存在
    for (auto& io_connect : io_2_io_list_)
    {
        if (true == compare_connect_io(io_connect.from_io_, io_connect.from_io_type_, from_io, io_type))
        {
            io_connect.from_session_id_ = session_id;
        }

        if (true == compare_connect_io(io_connect.to_io_, io_connect.to_io_type_, from_io, io_type))
        {
            io_connect.to_session_id_ = session_id;
        }

        if (io_connect.from_session_id_ > 0 && io_connect.to_session_id_ > 0)
        {
            //建立连接关系
            CIo_Session_to_Session s_2_s;
            s_2_s.from_session_id_ = io_connect.from_session_id_;
            s_2_s.to_session_id_ = io_connect.to_session_id_;
            session_to_session_list_.emplace_back(s_2_s);
            break;
        }
    }
}

void CIotoIo::unregedit_session_id(_ClientIPInfo from_io, EM_CONNECT_IO_TYPE io_type)
{
    //链接失效
    auto from_io_key = get_connect_list_key(from_io, io_type);

    auto f = connect_list_.find(from_io_key);
    if (f != connect_list_.end())
    {
        //删除链接消息
        auto session_id = f->second;
        connect_list_.erase(from_io_key);

        //寻找配置消息
        for (auto& connect_info : io_2_io_list_)
        {
            if (connect_info.from_session_id_ == session_id)
            {
                connect_info.from_session_id_ = 0;
                break;
            }

            if (connect_info.to_session_id_ == session_id)
            {
                connect_info.to_session_id_ = 0;
                break;
            }
        }

        //清理链接失败信息
        int nPos = 0;
        for (auto s_2_s : session_to_session_list_)
        {
            if (s_2_s.from_session_id_ == session_id || s_2_s.to_session_id_ == session_id)
            {
                session_to_session_list_.erase(session_to_session_list_.begin() + nPos);
                break;
            }
            nPos++;
        }
    }

}

uint32 CIotoIo::get_to_session_id(uint32 session_id)
{
    for (auto s_2_s : session_to_session_list_)
    {
        if (s_2_s.from_session_id_ == session_id)
        {
            return s_2_s.to_session_id_;
        }
        
        if (s_2_s.to_session_id_ == session_id)
        {
            return s_2_s.from_session_id_;
        }
    }

    return 0;
}

bool CIotoIo::compare_connect_io(_ClientIPInfo from_io, EM_CONNECT_IO_TYPE from_io_type, _ClientIPInfo target_io, EM_CONNECT_IO_TYPE target_io_type)
{
    if (from_io.m_strClientIP == target_io.m_strClientIP &&
        from_io.m_u2Port == target_io.m_u2Port &&
        from_io_type == target_io_type)
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint32 CIotoIo::get_regedit_session_id(_ClientIPInfo from_io, EM_CONNECT_IO_TYPE io_type)
{
    auto from_io_key = get_connect_list_key(from_io, io_type);

    auto f = connect_list_.find(from_io_key);
    if (f == connect_list_.end())
    {
        //没有找到在线的
        return 0;
    }
    else
    {
        //找到了在线的
        return f->second;
    }

}

std::string CIotoIo::get_connect_list_key(_ClientIPInfo from_io, EM_CONNECT_IO_TYPE io_type)
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
