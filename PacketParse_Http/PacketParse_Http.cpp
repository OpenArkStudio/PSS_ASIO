// PacketParse_Http.cpp :
// 进行http协议解析的类
//add by freeeyes

#include <iostream>
#include <map>

#include "define.h"
#include "SessionBuffer.hpp"
#include "HttpFormat.h"
#include "WebsocketFormat.hpp"

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

//协议类型和状态
enum class ENUM_Protocol
{
    PROTOCAL_HTTP_POST = 1,
    PROTOCAL_WEBSOCKET,
};

class CProtocalInfo
{
public:
    ENUM_Protocol protocol_ = ENUM_Protocol::PROTOCAL_HTTP_POST;
    shared_ptr<CHttpFormat> http_format_;
};

using map_http_parse = map<uint32, CProtocalInfo>;
map_http_parse map_http_parse_;
const uint16 http_post_command = 0x3001;
const uint16 http_websocket_shark_hand = 0x3002;
const uint16 websocket_data = 0x3003;

//处理http消息拆解
bool dispose_http_message(std::string http_request_text, CProtocalInfo& protocal_info, vector<CMessage_Packet>& message_list)
{
    int ret = protocal_info.http_format_->try_parse(http_request_text);
    if (ret == 0)
    {
        //完整的http数据包解析完成，组装逻辑数据包
        if (protocal_info.http_format_->is_websocket() == false)
        {
            CMessage_Packet logic_packet;
            logic_packet.command_id_ = http_post_command;
            logic_packet.buffer_ = protocal_info.http_format_->get_post_text();
            protocal_info.protocol_ = ENUM_Protocol::PROTOCAL_HTTP_POST;
            message_list.emplace_back(logic_packet);
        }
        else
        {
            //是websocket的握手协议
            CMessage_Packet logic_packet;
            logic_packet.command_id_ = http_websocket_shark_hand;
            logic_packet.buffer_ = WebSocketFormat::wsHandshake(protocal_info.http_format_->get_websocket_client_key());
            protocal_info.protocol_ = ENUM_Protocol::PROTOCAL_WEBSOCKET;
            message_list.emplace_back(logic_packet);
        }
    }
    else if (ret == -1)
    {
        //无效的数据，断开连接
        PSS_LOGGER_DEBUG("[parse_packet_from_recv_buffer]parse error={0}", protocal_info.http_format_->get_post_error());
        return false;
    }

    return true;
}

bool parse_packet_from_recv_buffer(uint32 connect_id, CSessionBuffer* buffer, vector<CMessage_Packet>& message_list, EM_CONNECT_IO_TYPE emIOType)
{
    std::string http_request_text;
    http_request_text.append(buffer->read(), buffer->get_write_size());
    buffer->move(buffer->get_write_size());

    //寻找对应有没有存在的http解析类
    auto f = map_http_parse_.find(connect_id);
    if (f != map_http_parse_.end())
    {
        auto& protocal_info = f->second;
        if (f->second.protocol_ == ENUM_Protocol::PROTOCAL_HTTP_POST)
        {
            //解析http协议
            return dispose_http_message(http_request_text, protocal_info, message_list);
        }
        else
        {
            //解析websocket
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
        message.buffer_ = f->second.http_format_->get_response_text(message.buffer_);
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
        CProtocalInfo protocal_info;
        auto http_parse_interface = std::make_shared<CHttpFormat>();
        http_parse_interface->init_http_setting();
        protocal_info.http_format_ = http_parse_interface;
        map_http_parse_[u4ConnectID] = protocal_info;
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

