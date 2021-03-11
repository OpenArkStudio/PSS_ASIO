// PacketParse_Http.cpp :
// 进行http协议解析的类
//add by freeeyes

#include <iostream>
#include <map>

#include "define.h"
#include "SessionBuffer.hpp"
#include "HttpFormat.h"

#if PSS_PLATFORM == PLATFORM_WIN
#ifdef PACKETPARSE_HTTP_EXPORTS
#define DECLDIR extern "C" _declspec(dllexport)
#else
#define DECLDIR extern "C"__declspec(dllimport)
#endif
#else
#define DECLDIR extern "C"
#endif

#include <vector>

using namespace std;

DECLDIR bool parse_packet_from_recv_buffer(uint32 connect_id, CSessionBuffer* buffer, vector<CMessage_Packet>& message_list, EM_CONNECT_IO_TYPE emIOType);
DECLDIR bool parse_packet_format_send_buffer(uint32 connect_id, CMessage_Packet& message, EM_CONNECT_IO_TYPE emIOType);
DECLDIR bool connect(uint32 connect_id, const _ClientIPInfo& remote_ip, const _ClientIPInfo& local_ip, EM_CONNECT_IO_TYPE emIOType);
DECLDIR void disConnect(uint32 connect_id, EM_CONNECT_IO_TYPE emIOType);
DECLDIR void set_output(shared_ptr<spdlog::logger> logger);
DECLDIR void packet_load();
DECLDIR void packet_close();

using map_http_parse = map<uint32, shared_ptr<CHttpFormat>>;
map_http_parse map_http_parse_;
const uint16 http_post_command = 0x3001;

bool parse_packet_from_recv_buffer(uint32 connect_id, CSessionBuffer* buffer, vector<CMessage_Packet>& message_list, EM_CONNECT_IO_TYPE emIOType)
{
    std::string http_request_text;
    http_request_text.append(buffer->read(), buffer->get_write_size());
    buffer->move(buffer->get_write_size());

    //寻找对应有没有存在的http解析类
    auto f = map_http_parse_.find(connect_id);
    if (f != map_http_parse_.end())
    {
        int ret = f->second->try_parse(http_request_text);
        if (ret == 0)
        {
            //完整的http数据包解析完成，组装逻辑数据包
            CMessage_Packet logic_packet;
            logic_packet.command_id_ = http_post_command;
            logic_packet.buffer_ = f->second->get_post_text();
            message_list.emplace_back(logic_packet);
        }
        else if (ret == -1)
        {
            //无效的数据，断开连接
            PSS_LOGGER_DEBUG("[parse_packet_from_recv_buffer]parse error={0}", f->second->get_post_error());
            return false;
        }
    }

    return true;
}

bool parse_packet_format_send_buffer(uint32 connect_id, CMessage_Packet& message, EM_CONNECT_IO_TYPE emIOType)
{
    //组装http发送数据包
    auto f = map_http_parse_.find(connect_id);
    if (f != map_http_parse_.end())
    {
        //找到对象了，格式化http消息
        message.buffer_ = f->second->get_respose_text(message.buffer_);
    }

    return true;
}

bool connect(uint32 u4ConnectID, const _ClientIPInfo& remote_ip, const _ClientIPInfo& local_ip, EM_CONNECT_IO_TYPE emIOType)
{
    PSS_UNUSED_ARG(emIOType);
    PSS_LOGGER_INFO("[Connect]u4ConnectID = {}, {}:{} ==> {}:{}",
        u4ConnectID,
        remote_ip.m_strClientIP,
        remote_ip.m_u2Port,
        local_ip.m_strClientIP,
        local_ip.m_u2Port);

    auto f = map_http_parse_.find(u4ConnectID);
    if (f == map_http_parse_.end())
    {
        auto http_parse_interface = std::make_shared<CHttpFormat>();
        http_parse_interface->init_http_setting();
        map_http_parse_[u4ConnectID] = http_parse_interface;
    }

    return true;
}

void disConnect(uint32 u4ConnectID, EM_CONNECT_IO_TYPE emIOType)
{
    PSS_UNUSED_ARG(emIOType);
    PSS_LOGGER_DEBUG("[DisConnect]u4ConnectID={}.",
        u4ConnectID);

    map_http_parse_.erase(u4ConnectID);
}

void set_output(shared_ptr<spdlog::logger> logger)
{
    //设置输出对象
    spdlog::set_default_logger(logger);
}

void packet_load()
{
    //Packet_Parse初始化调用
}

void packet_close()
{
    //Packet_Parse关闭调用
}

