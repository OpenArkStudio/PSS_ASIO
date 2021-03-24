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

DECLDIR bool parse_packet_from_recv_buffer(uint32 connect_id, CSessionBuffer* buffer, vector<std::shared_ptr<CMessage_Packet>>& message_list, EM_CONNECT_IO_TYPE emIOType);
DECLDIR bool parse_packet_format_send_buffer(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, EM_CONNECT_IO_TYPE emIOType);
DECLDIR bool connect(uint32 connect_id, const _ClientIPInfo& remote_ip, const _ClientIPInfo& local_ip, EM_CONNECT_IO_TYPE emIOType);
DECLDIR void disConnect(uint32 connect_id, EM_CONNECT_IO_TYPE emIOType);
DECLDIR void set_output(shared_ptr<spdlog::logger> logger);
DECLDIR void packet_load();
DECLDIR void packet_close();

//协议类型和状态
enum class ENUM_Protocol
{
    PROTOCAL_HTTP_POST = 1,
    PROTOCAL_WEBSOCKET_SHARK_HAND,
    PROTOCAL_WEBSOCKET_DATA,
};

class CProtocalInfo
{
public:
    ENUM_Protocol protocol_ = ENUM_Protocol::PROTOCAL_HTTP_POST;
    shared_ptr<CHttpFormat> http_format_;
};

using map_http_parse = map<uint32, CProtocalInfo>;
map_http_parse map_http_parse_;
const uint16 http_post_command = 0x1001;
const uint16 http_websocket_shark_hand = 0x1002;
const uint16 websocket_data = 0x1003;

//处理websocket数据帧
bool dispose_websocket_data_message(CSessionBuffer* buffer, CProtocalInfo& protocal_info, vector<std::shared_ptr<CMessage_Packet>>& message_list)
{
    //循环解析数据帧
    size_t left_len = buffer->get_write_size();
    std::string cache_frame;

    while (left_len > 0)
    {
        std::string parse_data = "";

        auto op_code = WebSocketFormat::WebSocketFrameType::ERROR_FRAME;
        size_t frame_size = 0;
        bool is_fin = false;

        if (!WebSocketFormat::wsFrameExtractBuffer(buffer->read(),
            left_len,
            parse_data,
            op_code,
            frame_size,
            is_fin))
        {
            // 如果没有解析出完整的ws frame 则退出函数
            return true;
        }


        // 如果当前fram的fin为false或者opcode为延续包
        // 则将当前frame的payload添加到cache
        if (!is_fin ||
            op_code == WebSocketFormat::WebSocketFrameType::CONTINUATION_FRAME)
        {
            cache_frame += parse_data;
            parse_data.clear();
        }
        // 如果当前fram的fin为false，并且opcode不为延续包
        // 则表示收到分段payload的第一个段(frame)，需要缓存当前frame的opcode
        if (!is_fin &&
            op_code != WebSocketFormat::WebSocketFrameType::CONTINUATION_FRAME)
        {
            cache_frame = "";
        }

        left_len -= frame_size;
        buffer->move(frame_size);

        if (!is_fin)
        {
            continue;
        }

        // 如果fin为true，并且opcode为延续包
        // 则表示分段payload全部接受完毕
        // 因此需要获取之前第一次收到分段frame的opcode作为整个payload的类型
        if (op_code == WebSocketFormat::WebSocketFrameType::CONTINUATION_FRAME)
        {
            if (!cache_frame.empty())
            {
                parse_data = cache_frame;
            }
        }

        //拼接消息字符串
        auto logic_packet = std::make_shared<CMessage_Packet>();
        logic_packet->command_id_ = websocket_data;
        logic_packet->buffer_ = parse_data;
        protocal_info.protocol_ = ENUM_Protocol::PROTOCAL_WEBSOCKET_DATA;
        message_list.emplace_back(logic_packet);

    }

    return true;
}

//处理http消息拆解
bool dispose_http_message(std::string http_request_text, CProtocalInfo& protocal_info, vector<std::shared_ptr<CMessage_Packet>>& message_list)
{
    int ret = protocal_info.http_format_->try_parse(http_request_text);
    if (ret == 0)
    {
        //完整的http数据包解析完成，组装逻辑数据包
        if (protocal_info.http_format_->is_websocket() == false)
        {
            auto logic_packet = std::make_shared<CMessage_Packet>();
            logic_packet->command_id_ = http_post_command;
            logic_packet->buffer_ = protocal_info.http_format_->get_post_text();
            protocal_info.protocol_ = ENUM_Protocol::PROTOCAL_HTTP_POST;
            message_list.emplace_back(logic_packet);
        }
        else
        {
            //是websocket的握手协议
            auto logic_packet = std::make_shared<CMessage_Packet>();
            logic_packet->command_id_ = http_websocket_shark_hand;
            logic_packet->buffer_ = WebSocketFormat::wsHandshake(protocal_info.http_format_->get_websocket_client_key());
            protocal_info.protocol_ = ENUM_Protocol::PROTOCAL_WEBSOCKET_SHARK_HAND;
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

//合并websocket发送帧
bool do_websocket_send_frame(std::string send_data, std::string& send_frame)
{
    WebSocketFormat::wsFrameBuild(send_data.c_str(),
        send_data.size(),
        send_frame,
        WebSocketFormat::WebSocketFrameType::TEXT_FRAME,
        true,
        false);

    return true;
}

bool parse_packet_from_recv_buffer(uint32 connect_id, CSessionBuffer* buffer, vector<std::shared_ptr<CMessage_Packet>>& message_list, EM_CONNECT_IO_TYPE emIOType)
{
    //寻找对应有没有存在的http解析类
    auto f = map_http_parse_.find(connect_id);
    if (f != map_http_parse_.end())
    {
        auto& protocal_info = f->second;
        if (f->second.protocol_ == ENUM_Protocol::PROTOCAL_HTTP_POST)
        {
            std::string http_request_text;
            http_request_text.append(buffer->read(), buffer->get_write_size());
            buffer->move(buffer->get_write_size());

            //解析http协议
            return dispose_http_message(http_request_text, protocal_info, message_list);
        }
        else if(f->second.protocol_ == ENUM_Protocol::PROTOCAL_WEBSOCKET_DATA)
        {
            //解析websocket
            return dispose_websocket_data_message(buffer, protocal_info, message_list);
        }
    }

    return true;
}

bool parse_packet_format_send_buffer(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, EM_CONNECT_IO_TYPE emIOType)
{
    //组装http发送数据包
    auto f = map_http_parse_.find(connect_id);
    if (f != map_http_parse_.end())
    {
        if (f->second.protocol_ == ENUM_Protocol::PROTOCAL_HTTP_POST)
        {
            //格式化http消息
            message->buffer_ = f->second.http_format_->get_response_text(message->buffer_);
        }
        else if (f->second.protocol_ == ENUM_Protocol::PROTOCAL_WEBSOCKET_SHARK_HAND)
        {
            //格式化协议升级信息
            message->buffer_ = f->second.http_format_->get_response_websocket_text(message->buffer_);
            f->second.protocol_ = ENUM_Protocol::PROTOCAL_WEBSOCKET_DATA;
        }
        else
        {
            //格式化websocket帧数据
            std::string frame_data;
            do_websocket_send_frame(message->buffer_, frame_data);
            message->buffer_ = frame_data;
        }
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

