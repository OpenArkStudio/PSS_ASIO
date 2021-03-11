#include "BaseCommand.h"

void CBaseCommand::Init(ISessionService* session_service)
{
    session_service_ = session_service;

    if (TEST_FRAME_WORK_FLAG == 1)
    {
        session_service_->create_frame_work_thread(plugin_test_logic_thread_id);

        CMessage_Packet send_message;
        CFrame_Message_Delay delay_timer;

        delay_timer.delay_seconds_ = std::chrono::seconds(5);
        delay_timer.timer_id_ = 1001;  //这个ID必须是全局唯一的

        send_message.command_id_ = COMMAND_TEST_FRAME;
        send_message.buffer_ = "freeeyes";
        session_service_->send_frame_message(plugin_test_logic_thread_id, "time loop", send_message, delay_timer);
    }

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
    io_info.server_port = 10005;
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

void CBaseCommand::logic_connect(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
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

        /*测试发送UDP消息*/
        if (TEST_FRAME_WORK_FLAG == 1)
        {
            CMessage_Packet send_asyn_packet;
            send_asyn_packet.command_id_ = 0x1002;
            send_asyn_packet.buffer_ = "111111";

            session_service_->send_io_message(source.connect_id_, send_asyn_packet);
        }
    }
}

void CBaseCommand::logic_disconnect(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    PSS_LOGGER_DEBUG("[do_module_message]connand={}, disconnect", source.connect_id_);
}

void CBaseCommand::logic_test_sync(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    //处理发送数据(同步)
    send_packet.buffer_.append(recv_packet.buffer_.c_str(), recv_packet.buffer_.size());
}

void CBaseCommand::logic_test_asyn(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    //处理发送数据(异步)
    CMessage_Packet send_asyn_packet;
    send_asyn_packet.buffer_.append(recv_packet.buffer_.c_str(), recv_packet.buffer_.size());

    session_service_->send_io_message(source.connect_id_, send_asyn_packet);
}

void CBaseCommand::logic_test_frame(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    //处理插件处理任务
    PSS_LOGGER_DEBUG("[logic_test_frame] tag_name={0},data={1}.", source.remote_ip_.m_strClientIP, recv_packet.buffer_);

    if (TEST_FRAME_WORK_FLAG == 1)
    {
        CMessage_Packet send_message;
        CFrame_Message_Delay delay_timer;

        delay_timer.delay_seconds_ = std::chrono::seconds(5);
        delay_timer.timer_id_ = 1001;  //这个ID必须是全局唯一的

        send_message.command_id_ = COMMAND_TEST_FRAME;
        send_message.buffer_ = "freeeyes";
        session_service_->send_frame_message(plugin_test_logic_thread_id, "time loop", send_message, delay_timer);

        //测试定时器(删除)
        if (TEST_FRAME_WORK_FLAG == 1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            session_service_->delete_frame_message_timer(1001);
        }
    }
}

void CBaseCommand::logic_test_connect_error(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    PSS_LOGGER_DEBUG("[CBaseCommand::logic_test_connect_error]{0}:{1}", 
        source.remote_ip_.m_strClientIP,
        source.remote_ip_.m_u2Port);
}

void CBaseCommand::logic_test_listen_error(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    PSS_LOGGER_DEBUG("[CBaseCommand::logic_test_listen_error]{0}:{1}",
        source.local_ip_.m_strClientIP,
        source.local_ip_.m_u2Port);
}

void CBaseCommand::logic_http_post(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    PSS_LOGGER_DEBUG("[logic_http_post]post data={0}", recv_packet.buffer_);
}
