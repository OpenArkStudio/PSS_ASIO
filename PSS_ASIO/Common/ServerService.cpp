#include "ServerService.h"

#if PSS_PLATFORM == PLATFORM_WIN
BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_CLOSE_EVENT)
    {
        //在这里处理窗口服务器关闭回收资源
        PSS_LOGGER_DEBUG("[ConsoleHandlerRoutine]server is close.");
        App_ServerService::instance()->stop_service();
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

    //读取配置文件
    server_config_.read_server_config_file();

    auto config_output = server_config_.get_config_console();

    //初始化输出
    Init_Console_Output(config_output.file_output_,
        config_output.file_count_,
        config_output.max_file_size_,
        config_output.file_name_,
        config_output.output_level_);

    //初始化PacketParse插件
    for (auto packet_parse : server_config_.get_config_packet_list())
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
        [&](std::error_code ec, int /*signo*/)
        {
            PSS_LOGGER_DEBUG("[signals] server is error({0}).", ec.message());
            io_context_.stop();
        });

    //初始化执行库
    App_WorkThreadLogic::instance()->init_work_thread_logic(server_config_.get_config_workthread().work_thread_count_);

    //初始化框架定时器
    App_TimerManager::instance()->Start();

    //测试Tcp监听
    for(auto tcp_server : server_config_.get_config_tcp_list())
    {
        auto tcp_service = make_shared<CTcpServer>(io_context_, 
            tcp_server.ip_, 
            tcp_server.port_, 
            tcp_server.packet_parse_id_,
            tcp_server.recv_buff_size_,
            tcp_server.send_buff_size_);
        tcp_service_list_.emplace_back(tcp_service);
    }

    //测试UDP监听
    for (auto udp_server : server_config_.get_config_udp_list())
    {
        auto udp_service = make_shared<CUdpServer>(io_context_, 
            udp_server.ip_,
            udp_server.port_,
            udp_server.packet_parse_id_,
            udp_server.recv_buff_size_,
            udp_server.send_buff_size_);
        udp_service_list_.emplace_back(udp_service);
    }

    //启动服务器间链接
    App_CommunicationService::instance()->init_communication_service(io_context_);

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
