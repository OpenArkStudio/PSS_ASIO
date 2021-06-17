#include "BaseCommand.h"

void CBaseCommand::Init(ISessionService* session_service)
{
    session_service_ = session_service;

    //得到服务器的所有监听消息
    std::vector<CConfigNetIO> io_list;
    session_service_->get_server_listen_info(io_list, EM_CONNECT_IO_TYPE::CONNECT_IO_TCP);
    for (const auto& io_type : io_list)
    {
        PSS_LOGGER_DEBUG("[CBaseCommand::Init]tcp listen {0}:{1}", io_type.ip_, io_type.port_);
    }

    session_service_->get_server_listen_info(io_list, EM_CONNECT_IO_TYPE::CONNECT_IO_UDP);
    for (const auto& io_type : io_list)
    {
        PSS_LOGGER_DEBUG("[CBaseCommand::Init]udp listen {0}:{1}", io_type.ip_, io_type.port_);
    }

    session_service_->get_server_listen_info(io_list, EM_CONNECT_IO_TYPE::CONNECT_IO_TTY);
    for (const auto& io_type : io_list)
    {
        PSS_LOGGER_DEBUG("[CBaseCommand::Init]tty listen {0}:{1}", io_type.ip_, io_type.port_);
    }
#ifdef GCOV_TEST
    session_service_->create_frame_work_thread(plugin_test_logic_thread_id);

    auto send_message = std::make_shared<CMessage_Packet>();
    CFrame_Message_Delay delay_timer;

    delay_timer.delay_seconds_ = std::chrono::seconds(1);
    delay_timer.timer_id_ = 1001;  //这个ID必须是全局唯一的

    send_message->command_id_ = COMMAND_TEST_FRAME;
    send_message->buffer_ = "freeeyes";
    session_service_->send_frame_message(plugin_test_logic_thread_id, "time loop", send_message, delay_timer);

    session_service_->run_work_thread_logic(plugin_test_logic_thread_id, delay_timer, [this]() {
        PSS_LOGGER_DEBUG("[run_work_thread_logic]arrived({0}).", session_service_->get_curr_thread_logic_id());
        });

    //测试连接tcp
    logic_connect_tcp();

    //测试数据透传接口
    _ClientIPInfo from_io;
    from_io.m_strClientIP = "127.0.0.1";
    from_io.m_u2Port = 10010;

    _ClientIPInfo to_io;
    to_io.m_strClientIP = "127.0.0.1";
    to_io.m_u2Port = 10003;

    session_service_->add_session_io_mapping(from_io,
        EM_CONNECT_IO_TYPE::CONNECT_IO_TCP,
        to_io,
        EM_CONNECT_IO_TYPE::CONNECT_IO_SERVER_TCP);
#endif

    PSS_LOGGER_DEBUG("[load_module]({0})io thread count.", session_service_->get_io_work_thread_count());
}

void CBaseCommand::logic_connect_tcp()
{
    //测试服务器间链接，链接本地10003端口
    CConnect_IO_Info io_info;
    EM_CONNECT_IO_TYPE io_type = EM_CONNECT_IO_TYPE::CONNECT_IO_TCP;

    io_info.send_size = 1024;
    io_info.recv_size = 1024;
    io_info.server_ip = "127.0.0.1";
    io_info.server_port = 10003;
    io_info.client_ip = "127.0.0.1";
    io_info.client_port = 10091;
    io_info.server_id = 1001;
    io_info.packet_parse_id = 1;

    session_service_->connect_io_server(io_info, io_type);
}

void CBaseCommand::logic_connect_udp()
{
    //测试服务器间链接，链接本地10003端口
    CConnect_IO_Info io_info;
    EM_CONNECT_IO_TYPE io_type = EM_CONNECT_IO_TYPE::CONNECT_IO_UDP;

    io_info.send_size = 1024;
    io_info.recv_size = 1024;
    io_info.server_ip = "127.0.0.1";
    io_info.server_port = 10005;
    io_info.server_id = 1002;
    io_info.packet_parse_id = 1;

    session_service_->connect_io_server(io_info, io_type);
}

void CBaseCommand::logic_connect(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
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
        //session_service_->close_io_session(source.connect_id_);
    }
    else if (source.type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_SERVER_UDP)
    {
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, CONNECT_IO_SERVER_UDP", source.connect_id_);
        PSS_LOGGER_DEBUG("[logic_connect]connand={}, server_id={}", source.connect_id_, source.connect_mark_id_);
    }

#ifdef GCOV_TEST
    //测试插件间调用
#if PSS_PLATFORM == PLATFORM_WIN
    std::string module_name = "Test_Logic.dll";
#else
    std::string module_name = "libTest_Logic.so";
#endif
    auto module_send_packet = std::make_shared<CMessage_Packet>();
    auto module_return_packet = std::make_shared<CMessage_Packet>();
    module_send_packet->command_id_ = 0x5000;
    session_service_->module_run(module_name, module_send_packet, module_return_packet);
#endif
}

void CBaseCommand::logic_disconnect(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    PSS_LOGGER_DEBUG("[do_module_message]connand={}, disconnect", source.connect_id_);
}

void CBaseCommand::logic_test_sync(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    //处理发送数据(同步)
    send_packet->buffer_.append(recv_packet->buffer_.c_str(), recv_packet->buffer_.size());

#ifdef GCOV_TEST
    if (source.type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_UDP)
    {
        session_service_->close_io_session(source.connect_id_);
    }
#endif
}

void CBaseCommand::logic_test_asyn(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    //处理发送数据(异步)
    auto send_asyn_packet = std::make_shared<CMessage_Packet>();
    send_asyn_packet->buffer_.append(recv_packet->buffer_.c_str(), recv_packet->buffer_.size());

    session_service_->send_io_message(source.connect_id_, send_asyn_packet);
}

void CBaseCommand::logic_test_frame(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    //处理插件处理任务
    PSS_LOGGER_DEBUG("[logic_test_frame] tag_name={0},data={1}.", source.remote_ip_.m_strClientIP, recv_packet->buffer_);

#ifdef GCOV_TEST
    auto send_message = std::make_shared<CMessage_Packet>();
    CFrame_Message_Delay delay_timer;

    //测试创建工作线程
    session_service_->create_frame_work_thread(1010);

    //测试投递线程
    delay_timer.delay_seconds_ = std::chrono::seconds(5);
    delay_timer.timer_id_ = 1002;  //这个ID必须是全局唯一的

    send_message->command_id_ = COMMAND_TEST_FRAME;
    send_message->buffer_ = "freeeyes";
    session_service_->run_work_thread_logic(1010, delay_timer, []() {
        PSS_LOGGER_DEBUG("[run_work_thread_logic]1010 is arrived.");
        });

    //测试定时器(删除)
    std::this_thread::sleep_for(std::chrono::seconds(1));
    session_service_->delete_frame_message_timer(1002);

    //测试关闭工作线程
    session_service_->close_frame_work_thread(1010);
#endif
}

void CBaseCommand::logic_test_connect_error(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    PSS_LOGGER_DEBUG("[CBaseCommand::logic_test_connect_error]{0}:{1}",
        source.remote_ip_.m_strClientIP,
        source.remote_ip_.m_u2Port);
}

void CBaseCommand::logic_test_listen_error(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    PSS_LOGGER_DEBUG("[CBaseCommand::logic_test_listen_error]{0}:{1}",
        source.local_ip_.m_strClientIP,
        source.local_ip_.m_u2Port);
}

void CBaseCommand::logic_http_post(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    PSS_LOGGER_DEBUG("[logic_http_post]post data={0}", recv_packet->buffer_);
    //返回http消息
    send_packet->buffer_ = recv_packet->buffer_;
}

void CBaseCommand::logic_http_websocket_shark_hand(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    PSS_LOGGER_DEBUG("[logic_http_websocket_shark_hand]server key={0}", recv_packet->buffer_);
    //返回http消息
    send_packet->buffer_ = recv_packet->buffer_;
}

void CBaseCommand::logic_http_websocket_data(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    PSS_LOGGER_DEBUG("[logic_http_websocket_data]source={0}, text data={1}", source.connect_id_, recv_packet->buffer_);
    //返回websocket消息(同步测试)
    //send_packet.buffer_ = recv_packet.buffer_;

    //返回websocket消息(异步测试)
    auto send_asyn_packet = std::make_shared<CMessage_Packet>();
    send_asyn_packet->buffer_.append(recv_packet->buffer_.c_str(), recv_packet->buffer_.size());

    session_service_->send_io_message(source.connect_id_, send_asyn_packet);

}

std::string CBaseCommand::do_logic_api(std::string api_param)
{
    PSS_LOGGER_DEBUG("[CBaseCommand::do_logic_api]{0}.", api_param);
    return api_param;
}
