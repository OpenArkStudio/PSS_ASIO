#pragma once

#include "define.h"

//add by freeeyes
//这里我想了良久，最后决定对IO桥接的效率放在第一位。
//那么这个接口应该交付给PacketParse去处理，connect和disconnect

//实现IO桥接的虚类
class IIoBridge
{
public:
    virtual bool add_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& to_io, EM_CONNECT_IO_TYPE to_io_type, ENUM_IO_BRIDGE_TYPE bridge_type = ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH) = 0;
    virtual bool delete_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type) = 0;
};
