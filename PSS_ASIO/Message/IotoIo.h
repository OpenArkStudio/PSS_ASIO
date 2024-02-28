#pragma once

//提供服务器到服务透传功能
//add by freeeyes

#include "define.h"
#include "spdlog/fmt/fmt.h"
#include "IIoBridge.hpp"
#include "ISession.h"

class CIo_Connect_Info
{
public:
    CIo_Connect_Info() = default;

    _ClientIPInfo from_io_;
    EM_CONNECT_IO_TYPE from_io_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_TCP;
    uint32 from_session_id_ = 0;
    std::shared_ptr<ISession> from_session_ = nullptr;
    _ClientIPInfo to_io_;
    EM_CONNECT_IO_TYPE to_io_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_TCP;
    uint32 to_session_id_ = 0;
    std::shared_ptr<ISession> to_session_ = nullptr;
    ENUM_IO_BRIDGE_TYPE bridge_type_ = ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH;
};

class CIotoIo 
{
public:
    //添加桥接设置
    bool add_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& to_io, EM_CONNECT_IO_TYPE to_io_type, ENUM_IO_BRIDGE_TYPE bridge_type);

    //删除桥接设置
    bool delete_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type);

    //当链接建立的时候处理
    void regedit_bridge_session_info(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id, std::shared_ptr<ISession> from_session);

    //当链接断开的时候处理
    void unregedit_bridge_session_info(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id);

private:
    bool compare_connect_io(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& target_io, EM_CONNECT_IO_TYPE target_io_type) const;

    void link_io_bridge(const CIo_Connect_Info& io_connect_info);

    void unlink_io_bridge(const CIo_Connect_Info& io_connect_info);

    int check_io_mapping(const CIo_Connect_Info&  connect_info);

    vector<CIo_Connect_Info> io_2_io_list_;
    std::mutex mutex_;
};
