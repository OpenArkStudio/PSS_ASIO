#include "LoadPacketParse.h"


void CLoadPacketParse::dispaly_error_message(const std::string func_name, const std::string packet_parse_file, std::shared_ptr<_Packet_Parse_Info> pPacketParseInfo)
{
    PSS_LOGGER_DEBUG("[CLoadPacketParse::dispaly_error_message] PacketID={0} strModuleName = {1}, Function {2} is error!", 
        pPacketParseInfo->m_u4PacketParseID,
        func_name,
        packet_parse_file, func_name);
    CLoadLibrary::PSS_dlClose(pPacketParseInfo->m_hModule);
}

bool CLoadPacketParse::LoadPacketInfo(uint32 u4PacketParseID, const std::string& packet_parse_path, const std::string& packet_parse_file)
{
    int nRet = 0;
    //隐式加载PacketParse接口
    auto pPacketParseInfo = std::make_shared<_Packet_Parse_Info>();

    if (nullptr == pPacketParseInfo)
    {
        PSS_LOGGER_DEBUG("[CLoadPacketParse::LoadPacketInfo] pPacketParseInfo is nullptr!");
        return false;
    }

    pPacketParseInfo->m_u4PacketParseID   = u4PacketParseID;
    string strFilePath;

    strFilePath = fmt::format("{0}{1}", packet_parse_path, packet_parse_file);
    pPacketParseInfo->m_hModule = CLoadLibrary::PSS_dlopen(strFilePath.c_str(), RTLD_NOW);

    if(nullptr == pPacketParseInfo->m_hModule)
    {
        PSS_LOGGER_DEBUG("[CLoadPacketParse::LoadPacketInfo] strModuleName = {0}, pModuleInfo->hModule is nullptr({1})!", packet_parse_file, CLoadLibrary::PSS_dlerror());
        return false;
    }

    pPacketParseInfo->packet_from_recv_buffer_ptr_ = (packet_from_recv_buffer)CLoadLibrary::PSS_dlsym(pPacketParseInfo->m_hModule, "parse_packet_from_recv_buffer");
    if(nullptr == pPacketParseInfo->m_hModule || !pPacketParseInfo->packet_from_recv_buffer_ptr_)
    {
        dispaly_error_message("parse_packet_from_recv_buffer", packet_parse_file, pPacketParseInfo);
        return false;
    }

    pPacketParseInfo->parse_format_send_buffer_ptr_ = (parse_format_send_buffer)CLoadLibrary::PSS_dlsym(pPacketParseInfo->m_hModule, "parse_packet_format_send_buffer");
    if (nullptr == pPacketParseInfo->m_hModule || !pPacketParseInfo->parse_format_send_buffer_ptr_)
    {
        dispaly_error_message("parse_packet_format_send_buffer", packet_parse_file, pPacketParseInfo);
        return false;
    }

    pPacketParseInfo->packet_connect_ptr_ = (packet_connect)CLoadLibrary::PSS_dlsym(pPacketParseInfo->m_hModule, "connect");

    if(nullptr == pPacketParseInfo->m_hModule || !pPacketParseInfo->packet_connect_ptr_)
    {
        dispaly_error_message("Connect", packet_parse_file, pPacketParseInfo);
        return false;
    }

    pPacketParseInfo->packet_disconnect_ptr_ = (packet_disconnect)CLoadLibrary::PSS_dlsym(pPacketParseInfo->m_hModule, "disConnect");

    if(nullptr == pPacketParseInfo->m_hModule || !pPacketParseInfo->packet_disconnect_ptr_)
    {
        dispaly_error_message("DisConnect", packet_parse_file, pPacketParseInfo);
        return false;
    }

    pPacketParseInfo->packet_load_ptr_ = (packet_load)CLoadLibrary::PSS_dlsym(pPacketParseInfo->m_hModule, "packet_load");

    if (nullptr == pPacketParseInfo->m_hModule || !pPacketParseInfo->packet_load_ptr_)
    {
        dispaly_error_message("Load", packet_parse_file, pPacketParseInfo);
        return false;
    }

    pPacketParseInfo->packet_close_ptr_ = (packet_close)CLoadLibrary::PSS_dlsym(pPacketParseInfo->m_hModule, "packet_close");

    if (nullptr == pPacketParseInfo->m_hModule || !pPacketParseInfo->packet_close_ptr_)
    {
        dispaly_error_message("Close", packet_parse_file, pPacketParseInfo);
        return false;
    }

    pPacketParseInfo->packet_set_output_ptr_ = (packet_set_output)CLoadLibrary::PSS_dlsym(pPacketParseInfo->m_hModule, "set_output");

    if (nullptr == pPacketParseInfo->m_hModule || !pPacketParseInfo->packet_set_output_ptr_)
    {
        dispaly_error_message("Set_output", packet_parse_file, pPacketParseInfo);
        return false;
    }

    //添加到HashPool里面
    m_objPacketParseList[pPacketParseInfo->m_u4PacketParseID] = pPacketParseInfo;

    //调用输出设置
    pPacketParseInfo->packet_set_output_ptr_(spdlog::default_logger());

    //调用初始化
    pPacketParseInfo->packet_load_ptr_();

    PSS_LOGGER_DEBUG("[CLoadPacketParse::LoadPacketInfo] load {0} OK!", packet_parse_file);

    return true;
}

shared_ptr<_Packet_Parse_Info> CLoadPacketParse::GetPacketParseInfo(uint32 u4PacketParseID)
{
    auto f = m_objPacketParseList.find(u4PacketParseID);

    if(m_objPacketParseList.end() == f)
    {
        //这里打印输出错误
        PSS_LOGGER_DEBUG("[CLoadPacketParse::GetPacketParseInfo]can't find u4PacketParseID({0}).", u4PacketParseID);
        return nullptr;
    }
    else
    {
        return f->second;
    }
}

void CLoadPacketParse::Close()
{
    PSS_LOGGER_DEBUG("[CLoadPacketParse::Close]Begin.");
    //清理所有已存在的指针
    for_each(m_objPacketParseList.begin(), m_objPacketParseList.end(), [](const std::pair<uint32, shared_ptr<_Packet_Parse_Info>>& iter) {
        //关闭模块接口
        iter.second->packet_close_ptr_();
        CLoadLibrary::PSS_dlClose(iter.second->m_hModule);
        });

    m_objPacketParseList.clear();
    PSS_LOGGER_DEBUG("[CLoadPacketParse::Close]End.");
}
