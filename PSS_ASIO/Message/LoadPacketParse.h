#ifndef _LOADPACKETPARSE_H
#define _LOADPACKETPARSE_H

#include "singleton.h"
#include <unordered_map>
#include "TimeStamp.hpp"
#include "LoadLibrary.hpp"
#include "SessionBuffer.hpp"

class _Packet_Parse_Info
{
public:
    uint32              m_u4PacketParseID     = 0;       //当前packetParseID
    PSS_Time_Point      m_tvCreateTime        = CTimeStamp::Get_Time_Stamp();          //模块创建时间
    Pss_Library_Handler m_hModule             = nullptr;
    bool (*Parse_Packet_From_Recv_Buffer)(uint32, CSessionBuffer* buffer, vector<CMessage_Packet>& message_list, EM_CONNECT_IO_TYPE emIOType)        = nullptr;
    bool (*Connect)(uint32 u4ConnectID, const _ClientIPInfo& objClientIPInfo, const _ClientIPInfo& objLocalIPInfo) = nullptr;
    void (*DisConnect)(uint32 u4ConnectID) = nullptr;
    void(*Load)() = nullptr;
    void(*Close)() = nullptr;
    void(*Set_output)(shared_ptr<spdlog::logger>) = nullptr;

    _Packet_Parse_Info() = default;
};

class CLoadPacketParse
{
public:
    CLoadPacketParse() = default;

    void Init(int nCount);

    bool LoadPacketInfo(uint32 u4PacketParseID, const char* pPacketParsePath, const char* szPacketParseName);

    void Close();

    shared_ptr<_Packet_Parse_Info> GetPacketParseInfo(uint32 u4PacketParseID);

private:
    using hashmapPacketParseModuleList = unordered_map<uint32, shared_ptr<_Packet_Parse_Info>>;
    hashmapPacketParseModuleList        m_objPacketParseList;                  //Hash内存池
    int m_nModuleCount = 0;
};

using App_PacketParseLoader = PSS_singleton<CLoadPacketParse>;

#endif
