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
#define DECLDIR 
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

//插件加载
int load_module(IFrame_Object* frame_object, string module_param)
{
    //注册插件
    frame_object->Regedit_command(COMMAND_TEST_SYNC);
    frame_object->Regedit_command(COMMAND_TEST_ASYN);

    PSS_LOGGER_DEBUG("[load_module]({0})finish.", module_param);

    return 0;
}

//卸载插件
void unload_module()
{
    PSS_LOGGER_DEBUG("[unload_module]finish.");
}

//执行消息处理
int do_module_message(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    //插件消息处理
    //PSS_LOGGER_DEBUG("[do_module_message]command_id={0}.", command_id);

    if (recv_packet.command_id_ == COMMAND_TEST_SYNC)
    {
        //处理数据
        send_packet.buffer_.append(recv_packet.buffer_.c_str(), recv_packet.buffer_.size());
    }

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

