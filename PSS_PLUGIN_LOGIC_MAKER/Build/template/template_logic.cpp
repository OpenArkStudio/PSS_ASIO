// Test_Logic.cpp : Define the logic plug-in call interface

#include <iostream>

#include "IFrameObject.h"
#include "define.h"

[include command file head file]

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
DECLDIR int do_module_message(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
DECLDIR int module_state();
DECLDIR int module_run(std::shared_ptr<CMessage_Packet> send_packet, std::shared_ptr<CMessage_Packet> return_packet);
DECLDIR void set_output(shared_ptr<spdlog::logger> logger);

ISessionService* session_service = nullptr;
std::shared_ptr<[command class name]> base_command = nullptr;

#define MESSAGE_FUNCTION_BEGIN(x) switch(x) {
#define MESSAGE_FUNCTION(x,y,z,h,i) case x: { y(z,h,i); break; }
#define MESSAGE_FUNCTION_END }

//插件加载
int load_module(IFrame_Object* frame_object, string module_param)
{
#ifdef GCOV_TEST
    //如果是功能代码覆盖率检查，则开启这个开关，让插件执行所有框架接口调用
    PSS_LOGGER_DEBUG("[load_module]gcov_check is set.");
#endif

    //初始化消息处理类
    base_command = std::make_shared<[command class name]>();

    //注册插件
    [regedit command id]

    session_service = frame_object->get_session_service();

    base_command->Init(session_service);

    PSS_LOGGER_DEBUG("[load_module]({0})finish.", module_param);

    return 0;
}

//卸载插件
void unload_module()
{
    PSS_LOGGER_DEBUG("[unload_module]finish.");
}

//执行消息处理
int do_module_message(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    //插件消息处理
    //PSS_LOGGER_DEBUG("[do_module_message]command_id={0}.", recv_packet.command_id_);

    MESSAGE_FUNCTION_BEGIN(recv_packet->command_id_);
    //MESSAGE_FUNCTION(LOGIC_COMMAND_CONNECT, base_command->logic_connect, source, recv_packet, send_packet);
    [message map logic funcion]
    MESSAGE_FUNCTION_END;

    return 0;
}

//模块间同步调用
int module_run(std::shared_ptr<CMessage_Packet> send_packet, std::shared_ptr<CMessage_Packet> return_packet)
{
    //这里添加你的逻辑处理代码
    PSS_LOGGER_DEBUG("[module_run]command_id_={0}.\n", send_packet->command_id_);
    return_packet->buffer_ = send_packet->buffer_;
    return_packet->command_id_ = send_packet->command_id_;
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

