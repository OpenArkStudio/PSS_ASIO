#ifdef SSL_SUPPORT
#include "TcpSSLClientSession.h"

CTcpSSLClientSession::CTcpSSLClientSession(asio::io_context* io_context) :
    io_context_(io_context),
    ssl_ctx_(asio::ssl::context(asio::ssl::context::sslv23)),
    ssl_socket_(*io_context, ssl_ctx_)
{
}

bool CTcpSSLClientSession::start(const CConnect_IO_Info& io_info)
{
    server_id_ = io_info.server_id;
    packet_parse_id_ = io_info.packet_parse_id;

    session_recv_buffer_.Init(io_info.recv_size);
    session_send_buffer_.Init(io_info.send_size);

    //验证证书的合法性
    asio::error_code pem_ec;

    PSS_LOGGER_DEBUG("[tcp_test_ssl_connect_synchronize_server]pem_ec={0}", io_info.client_pem_file);
    ssl_ctx_.set_verify_mode(asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert);
    ssl_ctx_.load_verify_file(io_info.client_pem_file, pem_ec);

    if (pem_ec)
    {
        PSS_LOGGER_DEBUG("[tcp_test_ssl_connect_synchronize_server]error={0}", pem_ec.message());
        return false;
    }

    ssl_socket_.set_verify_mode(asio::ssl::verify_peer);
    ssl_socket_.set_verify_callback(std::bind(&CTcpSSLClientSession::verify_certificate, this, _1, _2));

    //建立连接(异步)
    tcp::resolver resolver(*io_context_);
    auto endpoints = resolver.resolve(io_info.server_ip, std::to_string(io_info.server_port));
    asio::error_code connect_error;

    auto self(shared_from_this());
    asio::async_connect(ssl_socket_.lowest_layer(), endpoints,
        [self](const std::error_code& error,
            const tcp::endpoint& /*endpoint*/)
        {
            if (!error)
            {
                //连接远程SSL成功，进行异步握手
                self->handshake();
            }
            else
            {
                //连接远程SSL失败
                PSS_LOGGER_DEBUG("[CTcpSSLClientSession::start]Connect failed:{0}", error.message());

                //发送消息给逻辑块
                App_WorkThreadLogic::instance()->add_frame_events(LOGIC_CONNECT_SERVER_ERROR,
                    self->server_id_,
                    self->remote_ip_.m_strClientIP,
                    self->remote_ip_.m_u2Port,
                    self->io_type_);
            }
        });

    return true;
}

void CTcpSSLClientSession::close(uint32 connect_id)
{
    auto self(shared_from_this());

    auto recv_data_size = recv_data_size_;
    auto send_data_size = send_data_size_;

    EM_CONNECT_IO_TYPE io_type = io_type_;
    _ClientIPInfo remote_ip = remote_ip_;

    io_context_->dispatch([self, connect_id, io_type, remote_ip, recv_data_size, send_data_size]() 
        {
            self->ssl_socket_.lowest_layer().close();

            //输出接收发送字节数
            PSS_LOGGER_DEBUG("[CTcpSSLClientSession::Close]recv:{0}, send:{1}", recv_data_size, send_data_size);

            //断开连接
            self->packet_parse_interface_->packet_disconnect_ptr_(connect_id, io_type, App_IoBridge::instance());

            //发送链接断开消息
            App_WorkThreadLogic::instance()->delete_thread_session(connect_id, self);
        });
}

void CTcpSSLClientSession::set_write_buffer(uint32 connect_id, const char* data, size_t length)
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

void CTcpSSLClientSession::do_read()
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

    ssl_socket_.async_read_some(asio::buffer(session_recv_buffer_.get_curr_write_ptr(), session_recv_buffer_.get_buffer_size()),
        [self](std::error_code ec, std::size_t length)
        {
            self->do_read_some(ec, length);
        });
}

_ClientIPInfo CTcpSSLClientSession::get_remote_ip(uint32 connect_id)
{
    return remote_ip_;
}

void CTcpSSLClientSession::do_write_immediately(uint32 connect_id, const char* data, size_t length)
{
    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(data, length);
    send_buffer->buffer_length_ = length;

    auto self(shared_from_this());
    io_context_->dispatch([self, send_buffer, connect_id]()
        {
            //异步发送
            asio::async_write(self->ssl_socket_, asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_),
                [self, connect_id, send_buffer](std::error_code ec, std::size_t send_length)
                {
                    if (ec)
                    {
                        //发送给逻辑线程处理
                        PSS_LOGGER_DEBUG("[CTcpSSLClientSession::do_write_immediately]({0}), message({1})", connect_id, ec.message());

                        self->send_write_fail_to_logic(send_buffer->data_, send_length);
                    }
                    else
                    {
                        self->add_send_finish_size(connect_id, send_length);
                    }
                });
        });
}

void CTcpSSLClientSession::do_write(uint32 connect_id)
{
    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(session_send_buffer_.read(), session_send_buffer_.get_write_size());
    send_buffer->buffer_length_ = session_send_buffer_.get_write_size();

    clear_write_buffer();

    //异步发送
    auto self(shared_from_this());
    asio::async_write(ssl_socket_, asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_),
        [self, send_buffer, connect_id](std::error_code ec, std::size_t length)
        {
            if (ec)
            {
                //发送消息发送失败的信息给逻辑模块
                PSS_LOGGER_DEBUG("[CTcpSSLClientSession::do_write]({0})write error({1}).", connect_id, ec.message());
                self->send_write_fail_to_logic(send_buffer->data_, length);
            }
            else
            {
                self->add_send_finish_size(connect_id, length);
            }
        });
}

void CTcpSSLClientSession::add_send_finish_size(uint32 connect_id, size_t send_length)
{
    send_data_size_ += send_length;
}

EM_CONNECT_IO_TYPE CTcpSSLClientSession::get_io_type()
{
    return io_type_;
}

uint32 CTcpSSLClientSession::get_mark_id(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return server_id_;
}

uint32 CTcpSSLClientSession::get_connect_id() 
{
    return connect_id_;
}

void CTcpSSLClientSession::regedit_bridge_session_id(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return;
}

std::chrono::steady_clock::time_point& CTcpSSLClientSession::get_recv_time(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return recv_data_time_;
}

void CTcpSSLClientSession::set_io_bridge_connect_id(uint32 from_io_connect_id, uint32 to_io_connect_id)
{
    if (to_io_connect_id > 0)
    {
        io_state_ = EM_SESSION_STATE::SESSION_IO_BRIDGE;
        io_bridge_connect_id_ = from_io_connect_id;
    }
    else
    {
        io_state_ = EM_SESSION_STATE::SESSION_IO_LOGIC;
        io_bridge_connect_id_ = 0;
    }
}

bool CTcpSSLClientSession::format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message)
{
    return packet_parse_interface_->parse_format_send_buffer_ptr_(connect_id, message, format_message, get_io_type());
}

bool CTcpSSLClientSession::is_need_send_format()
{
    return packet_parse_interface_->is_need_send_format_ptr_();
}

void CTcpSSLClientSession::clear_write_buffer()
{
    session_send_buffer_.move(session_send_buffer_.get_write_size());
}

void CTcpSSLClientSession::do_read_some(std::error_code ec, std::size_t length)
{
    if (!ec)
    {
        recv_data_size_ += length;
        session_recv_buffer_.set_write_data(length);
        
        //判断是否有桥接
        if (EM_SESSION_STATE::SESSION_IO_BRIDGE == io_state_)
        {
            //将数据转发给桥接接口
            auto ret = App_WorkThreadLogic::instance()->do_io_bridge_data(connect_id_, io_bridge_connect_id_, session_recv_buffer_, length, shared_from_this());
            if (1 == ret)
            {
                //远程IO链接已断开
                io_bridge_connect_id_ = 0;
            }
        }
        else
        {
            //处理数据拆包
            vector<std::shared_ptr<CMessage_Packet>> message_list;
            bool ret = packet_parse_interface_->packet_from_recv_buffer_ptr_(connect_id_, &session_recv_buffer_, message_list, io_type_);
            if (!ret)
            {
                //链接断开(解析包不正确)
                App_WorkThreadLogic::instance()->close_session_event(connect_id_, shared_from_this());
            }
            else
            {
                recv_data_time_ = std::chrono::steady_clock::now();

                //添加消息处理
                App_WorkThreadLogic::instance()->assignation_thread_module_logic(connect_id_, message_list, shared_from_this());
            }

            session_recv_buffer_.move(length);
        }

        //继续读数据
        do_read();
    }
    else
    {
        //链接断开
        App_WorkThreadLogic::instance()->close_session_event(connect_id_, shared_from_this());
    }
}

void CTcpSSLClientSession::send_write_fail_to_logic(const std::string& write_fail_buffer, std::size_t buffer_length)
{
    vector<std::shared_ptr<CMessage_Packet>> message_ssl_connect_list;
    auto ssl_write_fail_packet = std::make_shared<CMessage_Packet>();
    ssl_write_fail_packet->command_id_ = LOGIC_THREAD_WRITE_IO_ERROR;
    ssl_write_fail_packet->buffer_.append(write_fail_buffer.c_str(), buffer_length);
    message_ssl_connect_list.emplace_back(ssl_write_fail_packet);

    //写IO失败消息提交给逻辑插件
    App_WorkThreadLogic::instance()->assignation_thread_module_logic_with_events(connect_id_, 
        message_ssl_connect_list, 
        shared_from_this());
}

bool CTcpSSLClientSession::verify_certificate(bool preverified, asio::ssl::verify_context& ctx)
{
    //证书验证
    char subject_name[500];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 500);
    PSS_LOGGER_DEBUG("[CTcpSSLClientSession::verify_certificate]server_id_={0},Verifying={1}", server_id_, subject_name);
    PSS_LOGGER_DEBUG("[CTcpSSLClientSession::verify_certificate]server_id_={0},preverified={1}", server_id_, preverified);

    return preverified;
}

void CTcpSSLClientSession::handshake()
{
    auto self(shared_from_this());
    ssl_socket_.async_handshake(asio::ssl::stream_base::client,
        [self](const std::error_code& error)
        {
            if (!error)
            {
                //握手成功，开始准备接收数据
                self->connect_id_ = App_ConnectCounter::instance()->CreateCounter();

                self->recv_data_time_ = std::chrono::steady_clock::now();

                self->packet_parse_interface_ = App_PacketParseLoader::instance()->GetPacketParseInfo(self->packet_parse_id_);

                //处理链接建立消息
                self->remote_ip_.m_strClientIP = self->ssl_socket_.lowest_layer().remote_endpoint().address().to_string();
                self->remote_ip_.m_u2Port = self->ssl_socket_.lowest_layer().remote_endpoint().port();
                self->local_ip_.m_strClientIP = self->ssl_socket_.lowest_layer().local_endpoint().address().to_string();
                self->local_ip_.m_u2Port = self->ssl_socket_.lowest_layer().local_endpoint().port();

                PSS_LOGGER_DEBUG("[CTcpSSLClientSession::start]remote({0}:{1})", self->remote_ip_.m_strClientIP, self->remote_ip_.m_u2Port);
                PSS_LOGGER_DEBUG("[CTcpSSLClientSession::start]local({0}:{1})", self->local_ip_.m_strClientIP, self->local_ip_.m_u2Port);

                self->packet_parse_interface_->packet_connect_ptr_(self->connect_id_, self->remote_ip_, self->local_ip_, self->io_type_, App_IoBridge::instance());

                //添加点对点映射
                if (true == App_IoBridge::instance()->regedit_bridge_session_id(self->remote_ip_, self->io_type_, self->connect_id_))
                {
                    self->io_state_ = EM_SESSION_STATE::SESSION_IO_BRIDGE;
                }

                //查看这个链接是否有桥接信息
                self->io_bridge_connect_id_ = App_IoBridge::instance()->get_to_session_id(self->connect_id_, self->remote_ip_);
                if (self->io_bridge_connect_id_ > 0)
                {
                    PSS_LOGGER_INFO("[CTcpSSLClientSession::handshake]connect_id={}, io_bridge_connect_id:{},the bridge is set successfully.", 
                        self->connect_id_, self->io_bridge_connect_id_);

                    //如果桥接成立，设置对端的桥接地址
                    App_WorkThreadLogic::instance()->set_io_bridge_connect_id(self->io_bridge_connect_id_, 
                        self->connect_id_,
                        App_IoBridge::instance()->find_io_bridge_type(self->connect_id_));
                }

                //添加映射关系
                App_WorkThreadLogic::instance()->add_thread_session(self->connect_id_, self, self->local_ip_, self->remote_ip_);

                self->do_read();
            }
            else
            {
                //握手失败
                PSS_LOGGER_DEBUG("[CTcpSSLClientSession::handshake]Handshake failed:{0}", error.message());
            }
        });
}

#endif
