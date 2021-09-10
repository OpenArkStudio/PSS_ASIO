#include "TcpClientSession.h"

#include "TcpSession.h"

CTcpClientSession::CTcpClientSession(asio::io_context* io_context)
    : socket_(*io_context)
{
}

bool CTcpClientSession::start(const CConnect_IO_Info& io_info)
{
    server_id_ = io_info.server_id;
    packet_parse_interface_ = App_PacketParseLoader::instance()->GetPacketParseInfo(io_info.packet_parse_id);

    session_recv_buffer_.Init(io_info.recv_size);
    session_send_buffer_.Init(io_info.send_size);

    //建立连接
    tcp::endpoint end_point(asio::ip::address::from_string(io_info.server_ip.c_str()), io_info.server_port);
    asio::error_code connect_error;

    //判断链接是否需要指定客户端IP和端口
    if (io_info.client_port > 0 && io_info.client_ip.length() > 0)
    {
        asio::ip::tcp::endpoint localEndpoint(asio::ip::address::from_string(io_info.client_ip), io_info.client_port);
        socket_.open(asio::ip::tcp::v4(), connect_error);
        socket_.bind(localEndpoint, connect_error);
    }

    //赋值对应的IP和端口信息
    local_ip_.m_strClientIP = io_info.client_ip;
    local_ip_.m_u2Port = io_info.client_port;
    remote_ip_.m_strClientIP = io_info.server_ip;
    remote_ip_.m_u2Port = io_info.server_port;

    //异步链接
    tcp::resolver::results_type::iterator endpoint_iter;
    socket_.async_connect(end_point, std::bind(&CTcpClientSession::handle_connect,
        this, std::placeholders::_1, endpoint_iter));

    return true;
}

void CTcpClientSession::close(uint32 connect_id)
{
    auto self(shared_from_this());
    socket_.close();

    //输出接收发送字节数
    PSS_LOGGER_DEBUG("[CTcpClientSession::Close]recv:{0}, send:{1}", recv_data_size_, send_data_size_);

    //断开连接
    packet_parse_interface_->packet_disconnect_ptr_(connect_id_, io_type_);

    //发送链接断开消息
    App_WorkThreadLogic::instance()->delete_thread_session(connect_id, remote_ip_, self);
}

void CTcpClientSession::set_write_buffer(uint32 connect_id, const char* data, size_t length)
{
    if (session_send_buffer_.get_buffer_size() <= length)
    {
        //发送些缓冲已经满了
        PSS_LOGGER_DEBUG("[CTcpSession::set_write_buffer]connect_id={} is full.", connect_id);
        return;
    }

    std::memcpy(session_send_buffer_.get_curr_write_ptr(),
        data,
        length);
    session_send_buffer_.set_write_data(length);
}

void CTcpClientSession::do_read()
{
    //接收数据
    auto self(shared_from_this());
    auto connect_id = connect_id_;

    //如果缓冲已满，断开连接，不再接受数据。
    if (session_recv_buffer_.get_buffer_size() == 0)
    {
        //链接断开(缓冲撑满了)
        App_tms::instance()->AddMessage(1, [self, connect_id]() {
            self->close(connect_id);
            });
    }

    socket_.async_read_some(asio::buffer(session_recv_buffer_.get_curr_write_ptr(), session_recv_buffer_.get_buffer_size()),
        [this, self](std::error_code ec, std::size_t length)
        {
            do_read_some(ec, length);
        });
}

void CTcpClientSession::do_write_immediately(uint32 connect_id, const char* data, size_t length)
{
    if (is_connect_ == false)
    {
        PSS_LOGGER_DEBUG("[CTcpClientSession::do_write_immediately]({0}), connect is not ready.", connect_id);
        return;
    }

    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(data, length);
    send_buffer->buffer_length_ = length;

    //异步发送
    auto self(shared_from_this());
    asio::async_write(socket_, asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_),
        [self, connect_id, send_buffer](std::error_code ec, std::size_t send_length)
        {
            if (ec)
            {
                //发送IO消息写入失败
                PSS_LOGGER_DEBUG("[CTcpClientSession::do_write_immediately]({0}), message({1})", connect_id, ec.message());
                self->send_write_fail_to_logic(send_buffer->data_, send_length);
            }
            else
            {
                self->add_send_finish_size(connect_id, send_length);
            }
        });
}

void CTcpClientSession::do_write(uint32 connect_id)
{
    if (is_connect_ == false)
    {
        PSS_LOGGER_DEBUG("[CTcpClientSession::do_write]({0}), connect is not ready.", connect_id);
        return;
    }

    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(session_send_buffer_.read(), session_send_buffer_.get_write_size());
    send_buffer->buffer_length_ = session_send_buffer_.get_write_size();

    clear_write_buffer();

    //异步发送
    auto self(shared_from_this());
    asio::async_write(socket_, asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_),
        [self, send_buffer, connect_id](std::error_code ec, std::size_t length)
        {
            if (ec)
            {
                //向逻辑线程投递发送失败消息
                PSS_LOGGER_DEBUG("[CTcpClientSession::do_write]write error({0}).", ec.message());
                self->send_write_fail_to_logic(send_buffer->data_, length);
            }
            else
            {
                self->add_send_finish_size(connect_id, length);
            }
        });
}

void CTcpClientSession::add_send_finish_size(uint32 connect_id, size_t send_length)
{
    send_data_size_ += send_length;
}

EM_CONNECT_IO_TYPE CTcpClientSession::get_io_type()
{
    return io_type_;
}

uint32 CTcpClientSession::get_mark_id(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return server_id_;
}

std::chrono::steady_clock::time_point& CTcpClientSession::get_recv_time()
{
    return recv_data_time_;
}

bool CTcpClientSession::format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message)
{
    return packet_parse_interface_->parse_format_send_buffer_ptr_(connect_id, message, format_message, get_io_type());
}

bool CTcpClientSession::is_need_send_format()
{
    return packet_parse_interface_->is_need_send_format_ptr_();
}

bool CTcpClientSession::is_connect()
{
    return is_connect_;
}

void CTcpClientSession::clear_write_buffer()
{
    session_send_buffer_.move(session_send_buffer_.get_write_size());
}

void CTcpClientSession::do_read_some(std::error_code ec, std::size_t length)
{
    if (!ec)
    {
        recv_data_size_ += length;
        session_recv_buffer_.set_write_data(length);
        PSS_LOGGER_DEBUG("[CTcpClientSession::do_write]recv length={}.", length);

        //处理数据拆包
        vector<std::shared_ptr<CMessage_Packet>> message_list;
        bool ret = packet_parse_interface_->packet_from_recv_buffer_ptr_(connect_id_, &session_recv_buffer_, message_list, io_type_);
        if (!ret)
        {
            //链接断开(解析包不正确)
            App_WorkThreadLogic::instance()->close_session_event(connect_id_);
        }
        else
        {
            recv_data_time_ = std::chrono::steady_clock::now();

            //添加消息处理
            App_WorkThreadLogic::instance()->assignation_thread_module_logic(connect_id_, message_list, shared_from_this());
        }

        session_recv_buffer_.move(length);
        //继续读数据
        do_read();
    }
    else
    {
        //链接断开
        App_WorkThreadLogic::instance()->close_session_event(connect_id_);
        is_connect_ = false;
    }
}

void CTcpClientSession::handle_connect(const asio::error_code& ec, tcp::resolver::results_type::iterator endpoint_iter)
{
    if (!ec)
    {
        is_connect_ = true;
        connect_id_ = App_ConnectCounter::instance()->CreateCounter();

        recv_data_time_ = std::chrono::steady_clock::now();

        //处理链接建立消息
        remote_ip_.m_strClientIP = socket_.remote_endpoint().address().to_string();
        remote_ip_.m_u2Port = socket_.remote_endpoint().port();
        local_ip_.m_strClientIP = socket_.local_endpoint().address().to_string();
        local_ip_.m_u2Port = socket_.local_endpoint().port();

        PSS_LOGGER_DEBUG("[CTcpClientSession::start]remote({0}:{1})", remote_ip_.m_strClientIP, remote_ip_.m_u2Port);
        PSS_LOGGER_DEBUG("[CTcpClientSession::start]local({0}:{1})", local_ip_.m_strClientIP, local_ip_.m_u2Port);

        packet_parse_interface_->packet_connect_ptr_(connect_id_, remote_ip_, local_ip_, io_type_);

#ifdef GCOV_TEST
        //测试发送写入失败回调消息
        std::string write_fail_text = "test write fail";
        send_write_fail_to_logic(write_fail_text, write_fail_text.length());
#endif

        //添加映射关系
        App_WorkThreadLogic::instance()->add_thread_session(connect_id_, shared_from_this(), local_ip_, remote_ip_);

        do_read();

    }
    else
    {
        is_connect_ = false;

        //连接建立失败
        PSS_LOGGER_DEBUG("[CTcpClientSession::start]({0}:{1}  ==> {2}:{3})error({4})", 
            local_ip_.m_strClientIP,
            local_ip_.m_u2Port,
            remote_ip_.m_strClientIP,
            remote_ip_.m_u2Port,
            ec.message());

        //发送消息给逻辑块
        App_WorkThreadLogic::instance()->add_frame_events(LOGIC_CONNECT_SERVER_ERROR,
            server_id_,
            remote_ip_.m_strClientIP,
            remote_ip_.m_u2Port,
            io_type_);
    }
}

void CTcpClientSession::send_write_fail_to_logic(const std::string& write_fail_buffer, std::size_t buffer_length)
{
    vector<std::shared_ptr<CMessage_Packet>> message_tcp_connect_list;
    auto write_fail_packet = std::make_shared<CMessage_Packet>();
    write_fail_packet->command_id_ = LOGIC_THREAD_WRITE_IO_ERROR;
    write_fail_packet->buffer_.append(write_fail_buffer.c_str(), buffer_length);
    message_tcp_connect_list.emplace_back(write_fail_packet);

    //写IO失败消息提交给逻辑插件
    App_WorkThreadLogic::instance()->assignation_thread_module_logic_with_events(connect_id_, 
        message_tcp_connect_list, 
        shared_from_this());
}

