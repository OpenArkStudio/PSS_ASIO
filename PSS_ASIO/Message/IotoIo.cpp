#include "IotoIo.h"

bool CIotoIo::add_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& to_io, EM_CONNECT_IO_TYPE to_io_type, ENUM_IO_BRIDGE_TYPE bridge_type)
{
    std::lock_guard <std::mutex> lock(mutex_);
    CIo_Connect_Info connect_info;
    connect_info.from_io_ = from_io;
    connect_info.from_io_type_ = from_io_type;
    connect_info.to_io_ = to_io;
    connect_info.to_io_type_ = to_io_type;
    connect_info.bridge_type_ = bridge_type;

    if(0 != this->check_io_mapping(connect_info))
    {
        return false;
    }

    //添加新的映射
    io_2_io_list_.emplace_back(connect_info);

    //这里需要注意，在add的时候，可能链接已经存在了，
    //这时候要在外面把这个链接信息注册到里面去。完成桥接。

    return true;
}

bool CIotoIo::delete_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type)
{
    //删除对应的点对点透传关系
    std::lock_guard <std::mutex> lock(mutex_);
    std::vector<CIo_Connect_Info> io_connect_info_list;
    
    int connect_pos = 0;
    bool is_break = false;
    for (const auto& io_connect : io_2_io_list_)
    {
        if (true == compare_connect_io(from_io, from_io_type, io_connect.from_io_, io_connect.from_io_type_))
        {
            //找到了，删除之
            io_connect_info_list.emplace_back(io_connect);
            io_2_io_list_.erase(io_2_io_list_.begin() + connect_pos);
            is_break = true;
        }

        if (is_break == true)
        {
            break;
        }

        connect_pos++;
    }

    connect_pos = 0;
    is_break = false;
    for (const auto& io_connect : io_2_io_list_)
    {
        if (true == compare_connect_io(from_io, from_io_type, io_connect.to_io_, io_connect.to_io_type_))
        {
            io_connect_info_list.emplace_back(io_connect);
            io_2_io_list_.erase(io_2_io_list_.begin() + connect_pos);
            is_break = true;
        }

        if (is_break == true)
        {
            break;
        }

        connect_pos++;
    }

    //取消桥接
    for (const auto& io_connect_info : io_connect_info_list)
    {
        unlink_io_bridge(io_connect_info);
    }

    return true;
}

void CIotoIo::link_io_bridge(const CIo_Connect_Info& io_connect_info)
{
    if (io_connect_info.from_session_id_ > 0 && io_connect_info.to_session_id_ > 0)
    {
        //如果两个链接都有，则需要根据类型来判定
        if (io_connect_info.bridge_type_ == ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH)
        {
            //需要两个都设置桥接
            io_connect_info.from_session_->set_io_bridge_connect_id(io_connect_info.from_session_id_, io_connect_info.to_session_id_);
            io_connect_info.to_session_->set_io_bridge_connect_id(io_connect_info.to_session_id_, io_connect_info.from_session_id_);
            PSS_LOGGER_DEBUG("[CIotoIo::link_io_bridge]from_id={}, to_id={} IO_BRIDGE_BATH is bridge.", 
                io_connect_info.from_session_id_,
                io_connect_info.to_session_id_);
        }
        else if (io_connect_info.bridge_type_ == ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_FROM)
        {
            //需要设置from
            io_connect_info.from_session_->set_io_bridge_connect_id(io_connect_info.from_session_id_, io_connect_info.to_session_id_);
            PSS_LOGGER_DEBUG("[CIotoIo::link_io_bridge]from_id={}, to_id={} IO_BRIDGE_FROM is bridge.",
                io_connect_info.from_session_id_,
                io_connect_info.to_session_id_);
        }
        else if (io_connect_info.bridge_type_ == ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_TO)
        {
            //需要设置to
            io_connect_info.to_session_->set_io_bridge_connect_id(io_connect_info.to_session_id_, io_connect_info.from_session_id_);
            PSS_LOGGER_DEBUG("[CIotoIo::link_io_bridge]from_id={}, to_id={} IO_BRIDGE_TO is bridge.",
                io_connect_info.from_session_id_,
                io_connect_info.to_session_id_);
        }
    }
}

void CIotoIo::regedit_bridge_session_info(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id, std::shared_ptr<ISession> from_session)
{
    std::lock_guard <std::mutex> lock(mutex_);
    std::vector<CIo_Connect_Info> io_connect_info_list;

    //寻找链接是否已经存在
    for (auto& io_connect : io_2_io_list_)
    {
        //先比较from是否存在，存在则放入列表
        if (true == compare_connect_io(io_connect.from_io_, io_connect.from_io_type_, from_io, io_type))
        {
            io_connect.from_session_id_ = session_id;
            io_connect.from_session_ = from_session;
            io_connect_info_list.push_back(io_connect);
        }

        //先比较to是否存在，存在则放入列表
        if (true == compare_connect_io(io_connect.to_io_, io_connect.to_io_type_, from_io, io_type))
        {
            io_connect.to_session_id_ = session_id;
            io_connect.to_session_ = from_session;
            io_connect_info_list.push_back(io_connect);
        }
    }

    //如果有则返回找到了，设置桥接
    for (const auto& io_connect_info : io_connect_info_list)
    {
        link_io_bridge(io_connect_info);
    }
}

void CIotoIo::unlink_io_bridge(const CIo_Connect_Info& io_connect_info)
{
    if (io_connect_info.from_session_id_ > 0 && io_connect_info.to_session_id_ > 0)
    {
        //如果两个链接都有，则需要根据类型来判定
        if (io_connect_info.bridge_type_ == ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH)
        {
            //需要两个都设置桥接
            io_connect_info.from_session_->set_io_bridge_connect_id(io_connect_info.from_session_id_, 0);
            io_connect_info.to_session_->set_io_bridge_connect_id(io_connect_info.to_session_id_, 0);
        }
        else if (io_connect_info.bridge_type_ == ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_FROM)
        {
            //需要设置from
            io_connect_info.from_session_->set_io_bridge_connect_id(io_connect_info.from_session_id_, 0);
        }
        else if (io_connect_info.bridge_type_ == ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_TO)
        {
            //需要设置to
            io_connect_info.to_session_->set_io_bridge_connect_id(io_connect_info.to_session_id_, 0);
        }
    }
}

void CIotoIo::unregedit_bridge_session_info(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id)
{
    std::lock_guard <std::mutex> lock(mutex_);
    std::vector<CIo_Connect_Info> io_connect_info_list;

    //删除对应的链接
    for (auto& io_connect : io_2_io_list_)
    {
        //先比较from是否存在，存在则放入列表
        if (true == compare_connect_io(io_connect.from_io_, io_connect.from_io_type_, from_io, io_type))
        {
            io_connect_info_list.push_back(io_connect);
            //删除对应桥接信息
            io_connect.from_session_id_ = 0;
            io_connect.from_session_ = nullptr;
        }

        //先比较to是否存在，存在则放入列表
        if (true == compare_connect_io(io_connect.to_io_, io_connect.to_io_type_, from_io, io_type))
        {
            io_connect_info_list.push_back(io_connect);
            //删除对应桥接信息
            io_connect.to_session_id_ = 0;
            io_connect.to_session_ = nullptr;
        }
    }

    //取消桥接
    for (const auto& io_connect_info : io_connect_info_list)
    {
        unlink_io_bridge(io_connect_info);
    }
}

bool CIotoIo::compare_connect_io(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& target_io, EM_CONNECT_IO_TYPE target_io_type) const
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

int CIotoIo::check_io_mapping(const CIo_Connect_Info& connect_info)
{
    for (const auto& Connect_Info : io_2_io_list_)
    {
        if (true == compare_connect_io(connect_info.from_io_, connect_info.from_io_type_, Connect_Info.from_io_, Connect_Info.from_io_type_))
        {
            //点对点链接已存在，不能添加
            PSS_LOGGER_WARN("[CIotoIo::check_io_mapping]io mapping duplication,from_io={}:{} is exist.", connect_info.from_io_.m_strClientIP, connect_info.from_io_.m_u2Port);
            return 1;
        }

        if (true == compare_connect_io(connect_info.to_io_, connect_info.to_io_type_, Connect_Info.from_io_, Connect_Info.from_io_type_))
        {
            //点对点链接已存在，不能添加
            PSS_LOGGER_WARN("[CIotoIo::check_io_mapping]io mapping loop,to_io={}:{} is exist.", connect_info.to_io_.m_strClientIP, connect_info.to_io_.m_u2Port);
            return 2;
        }
    }

    return 0;
}


