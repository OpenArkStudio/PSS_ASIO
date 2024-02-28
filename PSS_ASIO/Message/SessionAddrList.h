#pragma once

#include "define.h"
#include "ISession.h"

//实现对地址管理的遍历和搜索操作
//add by freeeyes

class CSessionAddr_Info
{
public:
    uint32 session_id = 0;
    std::shared_ptr<ISession> session_ = nullptr;
};

class CSessionAddrList
{
public:
    CSessionAddrList() = default;
    void add_session_addr(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id, std::shared_ptr<ISession> from_session);
    void del_session_addr(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type);

    CSessionAddr_Info get_session_addr_info(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type);

public:
    std::string set_addr_key(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type);

    using hashsessionaddrlist = unordered_map<std::string, CSessionAddr_Info>;
    hashsessionaddrlist session_addr_list_;
};
