#pragma once

//提供IO桥接服务注册
//add by freeeyes

#include "IotoIo.h" 
#include "SessionAddrList.h"

class CIoBridge : public IIoBridge
{
public:
    virtual ~CIoBridge() = default;

    bool add_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& to_io, EM_CONNECT_IO_TYPE to_io_type, ENUM_IO_BRIDGE_TYPE bridge_type = ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH) final;
    bool delete_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type) final;

    void regedit_bridge_session_info(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id, std::shared_ptr<ISession> from_session);
    void unregedit_bridge_session_info(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id);
private:
    CIotoIo iotoio_;
    CSessionAddrList sesion_addr_list_;
};

using App_IoBridge = PSS_singleton<CIoBridge>;
