// Test_Logic.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "IFrameObject.h"
#include "define.h"

#include <vector>

#if PSS_PLATFORM == PLATFORM_WIN
#ifdef TEST_LOGIC_EXPORTS
#define DECLDIR extern "C" _declspec(dllexport)
#else
#define DECLDIR extern "C"__declspec(dllimport)
#endif
#else
#define DECLDIR extern "C"
#endif

using namespace std;

DECLDIR int load_module(IFrame_Object* frame_object, string module_param);
DECLDIR void unload_module();
DECLDIR int do_module_message(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet);
DECLDIR int module_state();
DECLDIR void set_output(shared_ptr<spdlog::logger> logger);

//定义处理插件的command_id
const uint16 COMMAND_TEST_SYNC = 0x2101;
const uint16 COMMAND_TEST_ASYN = 0x2102;
const uint16 COMMAND_TEST_FRAME = 0x3100;

const uint32 plugin_test_logic_thread_id = 1001;

ISessionService* session_service = nullptr;

#define MESSAGE_FUNCTION_BEGIN(x) switch(x) {
#define MESSAGE_FUNCTION(x,y,z,h,i) case x: { y(z,h,i); break; }
#define MESSAGE_FUNCTION_END }

//测试逻辑代码（消息处理部分）
void logic_connect_tcp()
{
    //测试服务器间链接，链接本地10003端口
    CConnect_IO_Info io_info;
    EM_CONNECT_IO_TYPE io_type = EM_CONNECT_IO_TYPE::CONNECT_IO_TCP;

    io_info.send_size = 1024;
    io_info.recv_size = 1024;
    io_info.server_ip = "127.0.0.1";
    io_info.server_port = 10005;
    io_info.server_id = 1001;
    io_info.packet_parse_id = 1;

    session_service->connect_io_server(io_info, io_type);
}

void logic_connect_udp()
{
    //测试服务器间链接，链接本地10003端口
    CConnect_IO_Info io_info;
    EM_CONNECT_IO_TYPE io_type = EM_CONNECT_IO_TYPE::CONNECT_IO_UDP;

    io_info.send_size = 1024;
    io_info.recv_size = 1024;
    io_info.server_ip = "127.0.0.1";
    io_info.server_port = 10005;
    io_info.server_id = 1001;
    io_info.packet_parse_id = 1;

    session_service->connect_io_server(io_info, io_type);
}

//插件加载
int load_module(IFrame_Object* frame_object, string module_param)
{
    //注册插件
    frame_object->Regedit_command(LOGIC_COMMAND_CONNECT);
    frame_object->Regedit_command(LOGIC_COMMAND_DISCONNECT);
    frame_object->Regedit_command(COMMAND_TEST_SYNC);
    frame_object->Regedit_command(COMMAND_TEST_ASYN);
    frame_object->Regedit_command(COMMAND_TEST_FRAME);

    PSS_LOGGER_DEBUG("[load_module]({0})finish.", module_param);

    session_service = frame_object->get_session_service();

    session_service->create_frame_work_thread(plugin_test_logic_thread_id);

    CMessage_Packet send_message;
    send_message.command_id_ = COMMAND_TEST_FRAME;
    send_message.buffer_ = "freeeyes";
    session_service->send_frame_message(plugin_test_logic_thread_id, "time loop", send_message, std::chrono::seconds(5));

    return 0;
}

//卸载插件
void unload_module()
{
    PSS_LOGGER_DEBUG("[unload_module]finish.");
}

void logic_connect(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    PSS_LOGGER_DEBUG("[logic_connect]connand={}, connect", source.connect_id_);
    PSS_LOGGER_DEBUG("[logic_connect]connand={}, local ip={} local port={}", source.connect_id_, source.local_ip_.m_strClientIP, source.local_ip_.m_u2Port);
    PSS_LOGGER_DEBUG("[logic_connect]connand={}, remote ip={} remote port={}", source.connect_id_, source.remote_ip_.m_strClientIP, source.remote_ip_.m_u2Port);
    PSS_LOGGER_DEBUG("[logic_connect]connand={}, work thread id={}", source.connect_id_, source.work_thread_id_);

    if (source.type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_TCP)
    {
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, CONNECT_IO_TCP", source.connect_id_);
    }
    else if (source.type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_UDP)
    {
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, CONNECT_IO_UDP", source.connect_id_);
    }
    else if (source.type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_TTY)
    {
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, CONNECT_IO_TTY", source.connect_id_);
    }
    else if (source.type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_SERVER_TCP)
    {
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, CONNECT_IO_SERVER_TCP", source.connect_id_);
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, server_id={}", source.connect_id_, source.connect_mark_id_);

        //测试关闭链接
        //session_service->close_io_session(source.connect_id_);
    }
    else if (source.type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_SERVER_UDP)
    {
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, CONNECT_IO_SERVER_UDP", source.connect_id_);
    }
}

void logic_disconnect(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    PSS_LOGGER_DEBUG("[do_module_message]connand={}, disconnect", source.connect_id_);
}

void logic_test_sync(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    //处理发送数据(同步)
    send_packet.buffer_.append(recv_packet.buffer_.c_str(), recv_packet.buffer_.size());

    //测试服务器间链接
    //logic_connect_tcp();
}

void logic_test_asyn(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    //处理发送数据(异步)
    CMessage_Packet send_asyn_packet;
    send_asyn_packet.buffer_.append(recv_packet.buffer_.c_str(), recv_packet.buffer_.size());

    session_service->send_io_message(source.connect_id_, send_asyn_packet);
}

void logic_test_frame(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    //处理插件处理任务
    PSS_LOGGER_DEBUG("[logic_test_frame] tag_name={0},data={1}.", source.remote_ip_.m_strClientIP, recv_packet.buffer_);

    /*
    CMessage_Packet send_message;
    send_message.command_id_ = COMMAND_TEST_FRAME;
    send_message.buffer_ = "freeeyes";
    session_service->send_frame_message(plugin_test_logic_thread_id, "time loop", send_message, std::chrono::seconds(5));
    */
}

//执行消息处理
int do_module_message(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    //插件消息处理
    //PSS_LOGGER_DEBUG("[do_module_message]command_id={0}.", recv_packet.command_id_);

    MESSAGE_FUNCTION_BEGIN(recv_packet.command_id_);
    MESSAGE_FUNCTION(LOGIC_COMMAND_CONNECT, logic_connect, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(LOGIC_COMMAND_DISCONNECT, logic_disconnect, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(COMMAND_TEST_SYNC, logic_test_sync, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(COMMAND_TEST_ASYN, logic_test_asyn, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(COMMAND_TEST_FRAME, logic_test_frame, source, recv_packet, send_packet);
    MESSAGE_FUNCTION_END;

    return 0;
}

//获得当前插件状态
int module_state()
{
    return 0;
}

//设置日志输出
void set_output(shared_ptr<spdlog::logger> logger)
{
    //设置输出对象
    spdlog::set_default_logger(logger);
}

