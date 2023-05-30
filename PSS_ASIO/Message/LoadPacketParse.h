#pragma once

#include "singleton.h"
#include <unordered_map>
#include "TimeStamp.hpp"
#include "LoadLibrary.hpp"
#include "SessionBuffer.hpp"
#include "Iobridge.h"

using packet_from_recv_buffer = bool(*)(uint32 connectid, CSessionBuffer* buffer, vector<std::shared_ptr<CMessage_Packet>>& message_list, EM_CONNECT_IO_TYPE emIOType);
using parse_format_send_buffer = bool(*)(uint32 connectid, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message, EM_CONNECT_IO_TYPE emIOType);
using is_need_send_format = bool(*)();
using packet_connect = bool(*)(uint32 connectid, const _ClientIPInfo& objClientIPInfo, const _ClientIPInfo& objLocalIPInfo, EM_CONNECT_IO_TYPE emIOType, IIoBridge* io_bridge);
using packet_disconnect = void(*)(uint32 connectid, EM_CONNECT_IO_TYPE emIOType, IIoBridge* io_bridge);
using packet_load = void(*)(IIoBridge* io_bridge);
using packet_close = void(*)(IIoBridge* io_bridge);
using packet_set_output = void(*)(shared_ptr<spdlog::logger>);

class _Packet_Parse_Info
{
public:
    _Packet_Parse_Info() = default;

    uint32              m_u4PacketParseID     = 0;       //当前packetParseID
    PSS_Time_Point      m_tvCreateTime        = CTimeStamp::Get_Time_Stamp();          //模块创建时间
    Pss_Library_Handler m_hModule             = nullptr;
    packet_from_recv_buffer packet_from_recv_buffer_ptr_   = nullptr;
    parse_format_send_buffer parse_format_send_buffer_ptr_ = nullptr;
    packet_connect packet_connect_ptr_                     = nullptr;
    packet_disconnect packet_disconnect_ptr_               = nullptr;
    packet_load packet_load_ptr_                           = nullptr;
    packet_close packet_close_ptr_                         = nullptr;
    packet_set_output packet_set_output_ptr_               = nullptr;
    is_need_send_format is_need_send_format_ptr_           = nullptr;
};

class CLoadPacketParse
{
public:
    CLoadPacketParse() = default;

    void dispaly_error_message(const std::string& func_name, const std::string& packet_parse_file, std::shared_ptr<_Packet_Parse_Info> pPacketParseInfo) const;

    bool LoadPacketInfo(uint32 u4PacketParseID, const std::string& packet_parse_path, const std::string& packet_parse_file);

    void Close();

    shared_ptr<_Packet_Parse_Info> GetPacketParseInfo(uint32 u4PacketParseID);

private:
    using hashmapPacketParseModuleList = unordered_map<uint32, shared_ptr<_Packet_Parse_Info>>;
    hashmapPacketParseModuleList        m_objPacketParseList;                  //Hash内存池
};

using App_PacketParseLoader = PSS_singleton<CLoadPacketParse>;

