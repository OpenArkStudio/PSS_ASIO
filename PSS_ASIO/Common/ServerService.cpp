#include "ServerService.h"

#if PSS_PLATFORM == PLATFORM_WIN
BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_CLOSE_EVENT || dwCtrlType == CTRL_C_EVENT)
    {
        //在这里处理窗口服务器关闭回收资源
        PSS_LOGGER_DEBUG("[ConsoleHandlerRoutine]server is close.");
        App_ServerService::instance()->stop_service();
        return TRUE;
    }

    return FALSE;
}
#endif

#if PSS_PLATFORM == PLATFORM_UNIX
inline void daemonize()
{
    pid_t pid;

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    if (setpgrp() == -1)
    {
        perror("setpgrp failure");
    }

    signal(SIGHUP, SIG_IGN);

    if ((pid = fork()) < 0)
    {
        perror("fork failure");
        exit(1);
    }
    else if (pid > 0)
    {
        exit(0);
    }

    setsid();

    signal(SIGCLD, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
}
#endif

bool CServerService::init_service(const std::string& pss_config_file_name)
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
    //读取配置文件
    if (false == App_ServerConfig::instance()->read_server_config_file(pss_config_file_name))
    {
        return false;
    }

    PSS_LOGGER_DEBUG("[CServerService::init_service]configure file {0} read ok.", pss_config_file_name);

#if PSS_PLATFORM == PLATFORM_UNIX
    if (App_ServerConfig::instance()->get_config_workthread().linux_daemonize_ != 0)
    {
        //Linux 开启守护
        daemonize();
    }
#endif
    const auto& config_output = App_ServerConfig::instance()->get_config_console();

    //初始化输出
    Init_Console_Output(config_output.file_output_,
        config_output.file_count_,
        config_output.max_file_size_,
        config_output.file_name_,
        config_output.output_level_);

    PSS_LOGGER_DEBUG("[CServerService::init_service]build_time:{}\tversion:{}", V_BUILD_TIME, V_GIT_INFO);

#if PSS_PLATFORM == PLATFORM_WIN
    ::SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);
#endif

    App_IoContextPool::instance()->init();
    asio::io_context* io_contex = App_IoContextPool::instance()->getIOContext();
    //注册监控中断事件(LINUX)
    asio::signal_set signals(*io_contex, SIGINT, SIGTERM);
    signals.async_wait(
        [this](std::error_code ec, int)
        {
            PSS_LOGGER_DEBUG("[CServerService::init_service]server is error({0}).", ec.message());
            App_ServerService::instance()->stop_service();
        });

    //测试记录二进制
#ifdef GCOV_TEST
    char test_buffer[20] = { "freeeyes" };
    pss_output_binary(test_buffer, 0, 3);
#endif

    //初始化框架定时器
    App_TimerManager::instance()->Start();

    //初始化PacketParse插件
    for (const auto& packet_parse : App_ServerConfig::instance()->get_config_packet_list())
    {
        if (false == App_PacketParseLoader::instance()->LoadPacketInfo(packet_parse.packet_parse_id_,
            packet_parse.packet_parse_path_,
            packet_parse.packet_parse_file_name_))
        {
            PSS_LOGGER_DEBUG("[CServerService::init_service]load error.");
        }
    }

    //启动服务器间链接库
    App_CommunicationService::instance()->init_communication_service(CreateIoContextFunctor,
        (uint16)App_ServerConfig::instance()->get_config_workthread().s2s_timeout_seconds_);

    App_WorkThreadLogic::instance()->init_communication_service(App_CommunicationService::instance());

    //初始化执行库
    App_WorkThreadLogic::instance()->init_work_thread_logic(App_ServerConfig::instance()->get_config_workthread().work_thread_count_,
        (uint16)App_ServerConfig::instance()->get_config_workthread().work_timeout_seconds_,
        (uint32)App_ServerConfig::instance()->get_config_workthread().client_connect_timeout_,
        App_ServerConfig::instance()->get_config_logic_list(),
        App_SessionService::instance());

    //启动初始化配置好的网络服务
    App_NetSvrManager::instance()->start_default_service();
    
    //打开服务器间链接
    App_CommunicationService::instance()->run_server_to_server();

    App_IoContextPool::instance()->run();

    PSS_LOGGER_DEBUG("[CServerService::init_service] server is over.");

    return true;
}

void CServerService::close_service()
{
    PSS_LOGGER_DEBUG("[CServerService::close_service]begin.");

    //停止所有网络服务
    App_NetSvrManager::instance()->close_all_service();

    //关闭框架定时器
    App_TimerManager::instance()->Close();

    //停止服务间消息队列数据接收
    App_QueueSessionManager::instance()->close();

    App_SessionService::instance()->close();

    App_WorkThreadLogic::instance()->close();

    App_PacketParseLoader::instance()->Close();

    PSS_LOGGER_DEBUG("[CServerService::close_service]end.");
}

void CServerService::stop_service()
{
    PSS_LOGGER_DEBUG("[CServerService::stop_service]begin.");
    //停止所有的服务
    close_service();

    PSS_LOGGER_DEBUG("[CServerService::stop_service]end.");

    //停止，回收清理
    App_IoContextPool::instance()->stop();
}
