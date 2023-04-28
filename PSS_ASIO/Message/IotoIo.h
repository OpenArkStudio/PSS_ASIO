#pragma once

//提供服务器到服务透传功能
//add by freeeyes

#include "define.h"
#include "spdlog/fmt/fmt.h"

class CIo_Connect_Info
{
public:
    _ClientIPInfo from_io_;
    EM_CONNECT_IO_TYPE from_io_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_TCP;
    uint32 from_session_id_ = 0;
    _ClientIPInfo to_io_;
    EM_CONNECT_IO_TYPE to_io_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_TCP;
    uint32 to_session_id_ = 0;
    ENUM_IO_BRIDGE_TYPE bridge_type_ = ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH;
};

class CIo_Session_to_Session
{
public:
    uint32 from_session_id_ = 0;
    uint32 to_session_id_ = 0;
    _ClientIPInfo from_io_;
    _ClientIPInfo to_io_;
    ENUM_IO_BRIDGE_TYPE bridge_type_ = ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH;
};

class CIotoIo
{
public:
    bool add_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& to_io, EM_CONNECT_IO_TYPE to_io_type, ENUM_IO_BRIDGE_TYPE bridge_type);

    bool delete_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type);

    void regedit_session_id(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id);

    void unregedit_session_id(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type);

    uint32 get_to_session_id(uint32 session_id, const _ClientIPInfo& from_io);
private:
    uint32 get_endpoint_session_id(const _ClientIPInfo& from_io, const CIo_Session_to_Session& s_2_s);

    bool compare_connect_io(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& target_io, EM_CONNECT_IO_TYPE target_io_type) const;
 
    uint32 get_regedit_session_id(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type);

    std::string get_connect_list_key(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type) const;

    void delete_session_list(uint32 session_id);

    using hashmapconnectlist = unordered_map<std::string, uint32>;
    hashmapconnectlist connect_list_;
    vector<CIo_Connect_Info> io_2_io_list_;
    vector<CIo_Session_to_Session> session_to_session_list_;
    std::mutex mutex_;
};
