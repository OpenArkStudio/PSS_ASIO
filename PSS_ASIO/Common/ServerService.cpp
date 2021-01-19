#include "ServerService.h"

#if PSS_PLATFORM == PLATFORM_WIN
BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_CLOSE_EVENT)
    {
        //在这里处理窗口服务器关闭回收资源
        PSS_LOGGER_DEBUG("[ConsoleHandlerRoutine]server is close.");
        return TRUE;
    }

    return FALSE;
}
#endif

bool CServerService::init_servce()
{
    //指定当前目录，防止访问文件失败
#if PSS_PLATFORM == PLATFORM_WIN
    TCHAR szFileName[MAX_PATH] = { 0 };
    ::GetModuleFileName(0, szFileName, MAX_PATH);
    LPTSTR pszEnd = _tcsrchr(szFileName, TEXT('\\'));

    if (pszEnd != 0)
    {
        pszEnd++;
        *pszEnd = 0;
    }
#endif

    //初始化输出
    Init_Console_Output(0,
        1,
        1024000,
        "./serverlog",
        "debug");

    //初始化PacketParse插件
    if (false == App_PacketParseLoader::instance()->LoadPacketInfo(1, "./", "PacketParse_Inferface.dll"))
    {
        PSS_LOGGER_DEBUG("[App_PacketParseLoader] load error.");
    }

    int socket_serevr_port = 8888;

#if PSS_PLATFORM == PLATFORM_WIN
    ::SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);
#endif

    //注册监控中断事件(LINUX)
    asio::signal_set signals(io_context_, SIGINT, SIGTERM);
    signals.async_wait(
        [&](std::error_code ec, int /*signo*/)
        {
            PSS_LOGGER_DEBUG("[signals] server is error({0}).", ec.message());
            io_context_.stop();
        });

    //初始化执行库
    App_WorkThreadLogic::instance()->init_work_thread_logic(3);

    //测试Tcp监听
    auto tcp_service = make_shared<CTcpServer>(io_context_, "127.0.0.1", socket_serevr_port, 1, 102400);
    tcp_service_list_.emplace_back(tcp_service);

    //测试UDP监听
    //auto udp_service = make_shared<CUdpServer>(io_context_, "127.0.0.1", socket_serevr_port, 1, 1024);
    //udp_service_list_.emplace_back(udp_service);

    io_context_.run();

    PSS_LOGGER_DEBUG("[CServerService::init_servce] server is over.");
    close_service();

    return true;
}

void CServerService::close_service()
{
    PSS_LOGGER_DEBUG("[CServerService::close_service]begin.");
    //回收清理数据
    for (auto tcp_service : tcp_service_list_)
    {
        tcp_service->close();
    }

    tcp_service_list_.clear();

    App_WorkThreadLogic::instance()->close();
    PSS_LOGGER_DEBUG("[CServerService::close_service]end.");
}

void CServerService::stop_service()
{
    //停止，回收清理
    io_context_.stop();
}
