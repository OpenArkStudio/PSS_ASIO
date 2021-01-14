// PacketParse_Inferface.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "define.h"
#include "SessionBuffer.hpp"

#if PSS_PLATFORM == PLATFORM_WIN
#ifdef PACKETPARSE_INTERFACE_EXPORTS
#define DECLDIR extern "C" _declspec(dllexport)
#else
#define DECLDIR extern "C"__declspec(dllimport)
#endif
#else
#define DECLDIR 
#endif

#include <vector>

using namespace std;

DECLDIR bool Parse_Packet_From_Recv_Buffer(uint32 connect_id, CSessionBuffer* buffer, vector<CMessage_Packet>& message_list, EM_CONNECT_IO_TYPE emIOType);
DECLDIR bool Connect(uint32 connect_id, const _ClientIPInfo& remote_ip, const _ClientIPInfo& local_ip);
DECLDIR void DisConnect(uint32 connect_id);
DECLDIR void Set_output(shared_ptr<spdlog::logger> logger);
DECLDIR void Load();
DECLDIR void Close();


bool Parse_Packet_From_Recv_Buffer(uint32 connect_id, CSessionBuffer* buffer, vector<CMessage_Packet>& message_list, EM_CONNECT_IO_TYPE emIOType)
{
    uint32 packet_pos = 0;
    auto buff_length = buffer->get_write_size();
    auto pData = buffer->read();

    //在这里负责拆包
    while (true)
    {
        //数据包头是2+2+4+32 结构
        if (buff_length <= 40)
        {
            //包头不完整，不做解析
            buffer->move(packet_pos);
            break;
        }

        //如果有完整的包头
        uint32 packet_version     = 0;             //协议版本号
        uint32 command_id         = 0;             //CommandID
        uint32 packet_body_length = 0;             //包体长度
        char   packet_session[33] = { '\0' };      //Session字符串

        //继续偏移
        char* packet_buffer_data = pData + packet_pos;
        
        uint32 u4Pos = 0;

        //解析包头
        std::memcpy(&packet_version, &packet_buffer_data[u4Pos], (uint32)sizeof(uint16));
        u4Pos += sizeof(uint16);
        std::memcpy(&command_id, &packet_buffer_data[u4Pos], (uint32)sizeof(uint16));
        u4Pos += sizeof(uint16);
        std::memcpy(&packet_body_length, &packet_buffer_data[u4Pos], (uint32)sizeof(uint32));
        u4Pos += sizeof(uint32);
        std::memcpy(&packet_session, &packet_buffer_data[u4Pos], (uint32)(sizeof(char) * 32));
        u4Pos += sizeof(char) * 32;

        //判断包体长度是否大于指定的长度
        if (packet_body_length >= 1024000)
        {
            //非法数据包，返回失败，断开连接
            return false;
        }

        //如果包体长度为0
        if (packet_body_length == 0)
        {
            //拼接完整包，放入整包处理结构
            CMessage_Packet logic_packet;
            logic_packet.connect_id_ = connect_id;
            logic_packet.type_ = emIOType;
            logic_packet.command_id_ = command_id;
            logic_packet.buffer_.append(&packet_buffer_data[0], (size_t)40);
            message_list.emplace_back(logic_packet);

            uint32 curr_packet_size = 40;
            packet_pos += curr_packet_size;
            buff_length -= curr_packet_size;
        }
        else
        {
            if (buff_length < packet_body_length)
            {
                //收包不完整，继续接收
                buffer->move(packet_pos);
                break;
            }
            else
            {
                //拼接完整包，放入整包处理结构
                CMessage_Packet logic_packet;
                logic_packet.connect_id_ = connect_id;
                logic_packet.type_ = emIOType;
                logic_packet.command_id_ = command_id;
                logic_packet.buffer_.append(&packet_buffer_data[0], (size_t)40 + packet_body_length);
                message_list.emplace_back(logic_packet);

                uint32 curr_packet_size = 40 + packet_body_length;
                packet_pos += curr_packet_size;
                buff_length -= curr_packet_size;
            }
        }
    }

    return true;
}

bool Connect(uint32 u4ConnectID, const _ClientIPInfo& remote_ip, const _ClientIPInfo& local_ip)
{

    PSS_LOGGER_INFO("[Connect]{}:{} ==> {}{}",
        remote_ip.m_strClientIP,
        remote_ip.m_u2Port,
        local_ip.m_strClientIP,
        local_ip.m_u2Port);
    return true;
}

void DisConnect(uint32 u4ConnectID)
{
    PSS_LOGGER_DEBUG("[DisConnect]u4ConnectID={}.",
        u4ConnectID);
}

void Set_output(shared_ptr<spdlog::logger> logger)
{
    //设置输出对象
    spdlog::set_default_logger(logger);
}

void Load()
{
    //Packet_Parse初始化调用
}

void Close()
{
    //Packet_Parse关闭调用
}

