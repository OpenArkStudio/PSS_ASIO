// Test_Logic.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "IFrameObject.h"
#include "define.h"

#include <vector>

#include "BaseCommand.h"

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
DECLDIR int do_module_message(const CMessage_Source& source, const CMessage_Packet& recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
DECLDIR int module_state();
DECLDIR void set_output(shared_ptr<spdlog::logger> logger);

ISessionService* session_service = nullptr;
std::shared_ptr<CBaseCommand> base_command = nullptr;

#define MESSAGE_FUNCTION_BEGIN(x) switch(x) {
#define MESSAGE_FUNCTION(x,y,z,h,i) case x: { y(z,h,i); break; }
#define MESSAGE_FUNCTION_END }

//插件加载
int load_module(IFrame_Object* frame_object, string module_param)
{
    //初始化消息处理类
    base_command = std::make_shared<CBaseCommand>();

    //注册插件
    frame_object->Regedit_command(LOGIC_COMMAND_CONNECT);
    frame_object->Regedit_command(LOGIC_COMMAND_DISCONNECT);
    frame_object->Regedit_command(LOGIC_CONNECT_SERVER_ERROR);
    frame_object->Regedit_command(LOGIC_LISTEN_SERVER_ERROR);
    frame_object->Regedit_command(COMMAND_TEST_SYNC);
    frame_object->Regedit_command(COMMAND_TEST_ASYN);
    frame_object->Regedit_command(COMMAND_TEST_FRAME);
    frame_object->Regedit_command(COMMAND_TEST_HTTP_POST);
    frame_object->Regedit_command(COMMAND_WEBSOCKET_SHARK_HAND);
    frame_object->Regedit_command(COMMAND_WEBSOCKET_DATA);

    session_service = frame_object->get_session_service();

    base_command->Init(session_service);

    /*
    if (TEST_FRAME_WORK_FLAG == 1)
    {
        base_command->logic_connect_tcp();
        base_command->logic_connect_udp();
    }
    */

    PSS_LOGGER_DEBUG("[load_module]({0})finish.", module_param);

    return 0;
}

//卸载插件
void unload_module()
{
    PSS_LOGGER_DEBUG("[unload_module]finish.");
}

//执行消息处理
int do_module_message(const CMessage_Source& source, const CMessage_Packet& recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    //插件消息处理
    //PSS_LOGGER_DEBUG("[do_module_message]command_id={0}.", recv_packet.command_id_);

    MESSAGE_FUNCTION_BEGIN(recv_packet.command_id_);
    MESSAGE_FUNCTION(LOGIC_COMMAND_CONNECT, base_command->logic_connect, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(LOGIC_COMMAND_DISCONNECT, base_command->logic_disconnect, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(LOGIC_CONNECT_SERVER_ERROR, base_command->logic_test_connect_error, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(LOGIC_LISTEN_SERVER_ERROR, base_command->logic_test_listen_error, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(COMMAND_TEST_SYNC, base_command->logic_test_sync, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(COMMAND_TEST_ASYN, base_command->logic_test_asyn, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(COMMAND_TEST_FRAME, base_command->logic_test_frame, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(COMMAND_TEST_HTTP_POST, base_command->logic_http_post, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(COMMAND_WEBSOCKET_SHARK_HAND, base_command->logic_http_websocket_shark_hand, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(COMMAND_WEBSOCKET_DATA, base_command->logic_http_websocket_data, source, recv_packet, send_packet);
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

