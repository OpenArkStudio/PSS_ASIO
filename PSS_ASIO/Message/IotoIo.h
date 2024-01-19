#pragma once

//提供服务器到服务透传功能
//add by freeeyes

#include "define.h"
#include "spdlog/fmt/fmt.h"
#include "IIoBridge.hpp"

class CIo_Connect_Info
{
public:
    CIo_Connect_Info() = default;

    _ClientIPInfo from_io_;
    EM_CONNECT_IO_TYPE from_io_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_TCP;
    uint32 from_session_id_ = 0;
    _ClientIPInfo to_io_;
    EM_CONNECT_IO_TYPE to_io_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_TCP;
    uint32 to_session_id_ = 0;
    ENUM_IO_BRIDGE_TYPE bridge_type_ = ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH;

    // 赋值运算符重载函数
    CIo_Connect_Info& operator = (const CIo_Connect_Info& other)
    {
        // 避免自赋值
        if (this != &other)
        {
            from_io_ = other.from_io_;
            from_io_type_ = other.from_io_type_;
            from_session_id_ = other.from_session_id_;
            to_io_ = other.to_io_;
            to_io_type_ = other.to_io_type_;
            to_session_id_ = other.to_session_id_;
            bridge_type_ = other.bridge_type_;
        }

        return *this;
    }
};

class CIotoIo 
{
public:
    bool add_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& to_io, EM_CONNECT_IO_TYPE to_io_type, ENUM_IO_BRIDGE_TYPE bridge_type);

    bool delete_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type);

    bool regedit_bridge_session_id(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id);

    void unregedit_bridge_session_id(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type);

    uint32 get_to_session_id(uint32 session_id, const _ClientIPInfo& from_io);

    CIo_Connect_Info find_io_to_io_session_info(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE& from_io_type);

    const CIo_Connect_Info* find_io_to_io_list(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE& from_io_type);
private:
    uint32 get_endpoint_session_id(uint32 session_id, const _ClientIPInfo& from_io, const CIo_Connect_Info& s_2_s);

    bool compare_connect_io(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& target_io, EM_CONNECT_IO_TYPE target_io_type) const;
 
    uint32 get_regedit_bridge_session_id(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type);

    std::string get_connect_list_key(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type) const;

    void delete_session_list(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type);

    void delete_connect_list(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type);

    using hashmapconnectlist = unordered_map<std::string, uint32>;
    hashmapconnectlist connect_list_;
    vector<CIo_Connect_Info> io_2_io_list_;
    vector<CIo_Connect_Info> session_to_session_list_;
    std::mutex mutex_;
};
