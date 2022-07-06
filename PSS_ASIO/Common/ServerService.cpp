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

bool CServerService::init_servce(const std::string& pss_config_file_name)
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

    PSS_LOGGER_DEBUG("[CServerService::init_servce]configure file {0} read ok.", pss_config_file_name);

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

    //初始化PacketParse插件
    for (const auto& packet_parse : App_ServerConfig::instance()->get_config_packet_list())
    {
        if (false == App_PacketParseLoader::instance()->LoadPacketInfo(packet_parse.packet_parse_id_, 
            packet_parse.packet_parse_path_,
            packet_parse.packet_parse_file_name_))
        {
            PSS_LOGGER_DEBUG("[App_PacketParseLoader] load error.");
        }
    }

#if PSS_PLATFORM == PLATFORM_WIN
    ::SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);
#endif

    //注册监控中断事件(LINUX)
    asio::signal_set signals(io_context_, SIGINT, SIGTERM);
    signals.async_wait(
        [this](std::error_code ec, int)
        {
            PSS_LOGGER_DEBUG("[signals] server is error({0}).", ec.message());
            io_context_.stop();
        });

    //测试记录二进制
#ifdef GCOV_TEST
    char test_buffer[20] = { "freeeyes" };
    pss_output_binary(test_buffer, 0, 3);
#endif
    
    //初始化框架定时器
    App_TimerManager::instance()->Start();

    //启动服务器间链接库
    App_CommunicationService::instance()->init_communication_service(&io_context_,
        (uint16)App_ServerConfig::instance()->get_config_workthread().s2s_timeout_seconds_);

    App_WorkThreadLogic::instance()->init_communication_service(App_CommunicationService::instance());

    //初始化执行库
    App_WorkThreadLogic::instance()->init_work_thread_logic(App_ServerConfig::instance()->get_config_workthread().work_thread_count_,
        (uint16)App_ServerConfig::instance()->get_config_workthread().work_timeout_seconds_,
        (uint32)App_ServerConfig::instance()->get_config_workthread().client_connect_timeout_,
        (uint16)App_ServerConfig::instance()->get_config_workthread().io_send_time_check_,
        App_ServerConfig::instance()->get_config_logic_list(),
        App_SessionService::instance());

    //加载Tcp监听
    for(auto tcp_server : App_ServerConfig::instance()->get_config_tcp_list())
    {
        if (tcp_server.ssl_server_password_ != ""
            && tcp_server.ssl_server_pem_file_ != ""
            && tcp_server.ssl_dh_pem_file_ != "")
        {
#ifdef SSL_SUPPORT
            auto tcp_ssl_service = make_shared<CTcpSSLServer>(io_context_,
                tcp_server.ip_,
                tcp_server.port_,
                tcp_server.packet_parse_id_,
                tcp_server.recv_buff_size_,
                tcp_server.ssl_server_password_,
                tcp_server.ssl_server_pem_file_,
                tcp_server.ssl_dh_pem_file_);
            tcp_ssl_service_list_.emplace_back(tcp_ssl_service);
#else
            PSS_LOGGER_DEBUG("[CServerService::init_servce]you must set SSL_SUPPORT macro on compilation options.");
#endif
        }
        else
        {
            //正常的tcp链接
            auto tcp_service = make_shared<CTcpServer>(io_context_,
                tcp_server.ip_,
                tcp_server.port_,
                tcp_server.packet_parse_id_,
                tcp_server.recv_buff_size_);
            tcp_service_list_.emplace_back(tcp_service);
        }
    }

    //加载UDP监听
    for (auto udp_server : App_ServerConfig::instance()->get_config_udp_list())
    {
        auto udp_service = make_shared<CUdpServer>(io_context_, 
            udp_server.ip_,
            udp_server.port_,
            udp_server.packet_parse_id_,
            udp_server.recv_buff_size_,
            udp_server.send_buff_size_);
        udp_service->start();
        udp_service_list_.emplace_back(udp_service);
    }

    //加载tty监听
    for (auto tty_server : App_ServerConfig::instance()->get_config_tty_list())
    {
        auto tty_service = make_shared<CTTyServer>(
            tty_server.packet_parse_id_,
            tty_server.recv_buff_size_,
            tty_server.send_buff_size_);
        tty_service->start(&io_context_, 
            tty_server.tty_name_, 
            (uint16)tty_server.tty_port_,
            (uint8)tty_server.char_size_,
            0);
        tty_service_list_.emplace_back(tty_service);
    }

    //打开服务器间链接
    App_CommunicationService::instance()->run_server_to_server();

    io_context_.run();

    PSS_LOGGER_DEBUG("[CServerService::init_servce] server is over.");
    close_service();

    return true;
}

void CServerService::close_service()
{
    PSS_LOGGER_DEBUG("[CServerService::close_service]begin.");

    //关闭框架定时器
    App_TimerManager::instance()->Close();

    //停止服务间消息队列数据接收
    App_QueueSessionManager::instance()->close();

    //停止所有的TCP监听(TCP)
    for (const auto& tcp_service : tcp_service_list_)
    {
        tcp_service->close();
    }

#ifdef SSL_SUPPORT
    //停止所有的SSL监听
    for (const auto& tcp_ssl_service : tcp_ssl_service_list_)
    {
        tcp_ssl_service->close();
    }
#endif

    tcp_service_list_.clear();

    App_SessionService::instance()->close();

    App_WorkThreadLogic::instance()->close();

    App_PacketParseLoader::instance()->Close();

    PSS_LOGGER_DEBUG("[CServerService::close_service]end.");
}

void CServerService::stop_service()
{
    //停止，回收清理
    io_context_.stop();
}
