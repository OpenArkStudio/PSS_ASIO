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
DECLDIR int do_module_message(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet);
DECLDIR int module_state();
DECLDIR int module_run(std::shared_ptr<CMessage_Packet> send_packet, std::shared_ptr<CMessage_Packet> return_packet);
DECLDIR void set_output(shared_ptr<spdlog::logger> logger);

ISessionService* session_service = nullptr;
std::shared_ptr<CBaseCommand> base_command = nullptr;

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
    base_command = std::make_shared<CBaseCommand>();

    //测试添加性能探针
    CRandom_Sample random_sample(100);
    auto time_cost_func = std::bind(&CBaseCommand::performace_check, base_command.get(), std::placeholders::_1, std::placeholders::_2);
    CPerformance_Check performance_check(random_sample, "load_module", time_cost_func);

    //注册插件
    frame_object->Regedit_command(LOGIC_COMMAND_CONNECT);
    frame_object->Regedit_command(LOGIC_COMMAND_DISCONNECT);
    frame_object->Regedit_command(LOGIC_CONNECT_SERVER_ERROR);
    frame_object->Regedit_command(LOGIC_LISTEN_SERVER_ERROR);
    //frame_object->Regedit_command(COMMAND_TEST_SYNC);
    //frame_object->Regedit_command(COMMAND_TEST_ASYN);
    frame_object->Regedit_command(COMMAND_TEST_FRAME);
    frame_object->Regedit_command(COMMAND_TEST_HTTP_POST);
    frame_object->Regedit_command(COMMAND_WEBSOCKET_SHARK_HAND);
    frame_object->Regedit_command(COMMAND_WEBSOCKET_DATA);
    frame_object->Regedit_command(LOGIC_THREAD_DEAD_LOCK);
    frame_object->Regedit_command(LOGIC_THREAD_WRITE_IO_ERROR);

    //测试直接调用回调函数
    auto command_asyn_api = std::bind(&CBaseCommand::logic_test_asyn, 
        base_command.get(), 
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3);
    frame_object->Regedit_command(COMMAND_TEST_ASYN, command_asyn_api);

    auto command_sync_api = std::bind(&CBaseCommand::logic_test_sync,
        base_command.get(),
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3);
    frame_object->Regedit_command(COMMAND_TEST_SYNC, command_sync_api);


    session_service = frame_object->get_session_service();

    base_command->Init(session_service);

    //测试注册api
    auto test_api = std::bind(&CBaseCommand::do_logic_api, base_command.get(), std::placeholders::_1);
    session_service->add_plugin_api("test_logic", test_api);

    session_service->do_plugin_api("test_logic", "hello free eyes");

    PSS_LOGGER_DEBUG("[load_module]({0})finish.", module_param);

    //测试服务器间通讯
    shm_queue::shm_key key = 11111;
    bool ret = session_service->create_queue(key, 100, 10);
    if (ret)
    {
        session_service->set_recv_function(key, [](const char* message, size_t len) {
            PSS_LOGGER_DEBUG("[shm que message]message={0},len={1}.", message, len);
            });

        session_service->set_close_function(key, [](shm_queue::shm_key key) {
            PSS_LOGGER_DEBUG("[shm que message]close {0}.", key);
            });

        session_service->set_error_function(key, [](std::string error) {
            PSS_LOGGER_DEBUG("[shm que message]error={0}.", error);
            });

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        char buffer[30] = { "hello queue!" };
        if (!session_service->send_queue_message(key, buffer, strlen(buffer)))
        {
            PSS_LOGGER_DEBUG("[shm que message]put message error.");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        session_service->close(key);
    }

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
    MESSAGE_FUNCTION(LOGIC_COMMAND_CONNECT, base_command->logic_connect, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(LOGIC_COMMAND_DISCONNECT, base_command->logic_disconnect, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(LOGIC_CONNECT_SERVER_ERROR, base_command->logic_test_connect_error, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(LOGIC_LISTEN_SERVER_ERROR, base_command->logic_test_listen_error, source, recv_packet, send_packet);
    //MESSAGE_FUNCTION(COMMAND_TEST_SYNC, base_command->logic_test_sync, source, recv_packet, send_packet);
    //MESSAGE_FUNCTION(COMMAND_TEST_ASYN, base_command->logic_test_asyn, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(COMMAND_TEST_FRAME, base_command->logic_test_frame, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(COMMAND_TEST_HTTP_POST, base_command->logic_http_post, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(COMMAND_WEBSOCKET_SHARK_HAND, base_command->logic_http_websocket_shark_hand, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(COMMAND_WEBSOCKET_DATA, base_command->logic_http_websocket_data, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(LOGIC_THREAD_DEAD_LOCK, base_command->logic_work_thread_is_lock, source, recv_packet, send_packet);
    MESSAGE_FUNCTION(LOGIC_THREAD_WRITE_IO_ERROR, base_command->logic_io_write_error, source, recv_packet, send_packet);
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

