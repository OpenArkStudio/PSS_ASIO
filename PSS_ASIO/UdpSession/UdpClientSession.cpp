#include "UdpClientSession.h"

CUdpClientSession::CUdpClientSession(asio::io_context* io_context)
    : socket_(*io_context), io_context_(io_context)
{
}

void CUdpClientSession::start(const CConnect_IO_Info& io_type)
{
    session_recv_buffer_.Init(io_type.recv_size);
    session_send_buffer_.Init(io_type.send_size);

    server_id_ = io_type.server_id;

    //建立连接
    udp::endpoint end_point(asio::ip::address::from_string(io_type.server_ip.c_str()), io_type.server_port);
    send_endpoint_ = end_point;
    
    socket_.open(udp::v4());
    socket_.set_option(asio::ip::udp::socket::reuse_address(true));
    //判断有没有本地IP
    if (io_type.client_ip.size() > 0 && io_type.client_port > 0)
    {
        udp::endpoint local_endpoint(asio::ip::address::from_string(io_type.client_ip.c_str()), io_type.client_port); // 本地地址和端口，0.0.0.0表示任意地址
        recv_endpoint_ = local_endpoint;
        try
        {
            socket_.bind(local_endpoint);    // 将套接字绑定到本地地址和端口
        }
        catch (std::system_error const& ex)
        {
            PSS_LOGGER_ERROR("[CUdpClientSession::start] bind addr error remote:[{}:{}] local:[{}:{}] ex.what:{}.", io_type.server_ip, io_type.server_port,io_type.client_ip,io_type.client_port, ex.what());
        }
    }

    asio::error_code connect_error;
    
    socket_.connect(end_point, connect_error);

    if (connect_error)
    {
        //连接建立失败
        PSS_LOGGER_DEBUG("[CUdpClientSession::start]error({})", connect_error.message());
    }
    else
    {
        connect_id_ = App_ConnectCounter::instance()->CreateCounter();
        recv_data_time_ = std::chrono::steady_clock::now();

        packet_parse_interface_ = App_PacketParseLoader::instance()->GetPacketParseInfo(io_type.packet_parse_id);

        //处理链接建立消息
        _ClientIPInfo remote_ip;
        _ClientIPInfo local_ip;
        remote_ip.m_strClientIP = socket_.remote_endpoint().address().to_string();
        remote_ip.m_u2Port = socket_.remote_endpoint().port();
        local_ip.m_strClientIP = socket_.local_endpoint().address().to_string();
        local_ip.m_u2Port = socket_.local_endpoint().port();

        PSS_LOGGER_DEBUG("[CUdpClientSession::start]remote({0}:{1})", remote_ip.m_strClientIP, remote_ip.m_u2Port);
        PSS_LOGGER_DEBUG("[CUdpClientSession::start]local({0}:{1})", local_ip.m_strClientIP, local_ip.m_u2Port);

        packet_parse_interface_->packet_connect_ptr_(connect_id_, remote_ip, local_ip, io_type_, App_IoBridge::instance());

        //添加点对点映射
        if (true == App_IoBridge::instance()->regedit_session_id(remote_ip, io_type_, connect_id_))
        {
            io_state_ = EM_SESSION_STATE::SESSION_IO_BRIDGE;
        }

        //查看这个链接是否有桥接信息
        io_bridge_connect_id_ = App_IoBridge::instance()->get_to_session_id(connect_id_, remote_ip);
        if (io_bridge_connect_id_ > 0)
        {
            App_WorkThreadLogic::instance()->set_io_bridge_connect_id(connect_id_, io_bridge_connect_id_);
        }

        //添加映射关系
        App_WorkThreadLogic::instance()->add_thread_session(connect_id_, shared_from_this(), local_ip, remote_ip);

        do_receive();
    }
}

void CUdpClientSession::close(uint32 connect_id)
{
    auto self(shared_from_this());

    auto io_type = io_type_;

    io_context_->dispatch([self, connect_id, io_type]()
        {
            //删除映射关系
            _ClientIPInfo remote_ip;
            remote_ip.m_strClientIP = self->send_endpoint_.address().to_string();
            remote_ip.m_u2Port = self->send_endpoint_.port();

            self->socket_.close();

            self->packet_parse_interface_->packet_disconnect_ptr_(connect_id, io_type, App_IoBridge::instance());

            //输出接收发送字节数
            PSS_LOGGER_DEBUG("[CUdpClientSession::Close]recv:{0}, send:{1}", 
                self->recv_data_size_,
                self->send_data_size_);

            App_WorkThreadLogic::instance()->delete_thread_session(connect_id, self);
        });
}


void CUdpClientSession::do_receive()
{
    //接收数据

    //如果缓冲已满，断开连接，不再接受数据。
    if (session_recv_buffer_.get_buffer_size() == 0)
    {
        //链接断开(缓冲撑满了)
        App_WorkThreadLogic::instance()->close_session_event(connect_id_);
    }

    auto self(shared_from_this());
    socket_.async_receive_from(asio::buffer(session_recv_buffer_.get_curr_write_ptr(), session_recv_buffer_.get_buffer_size()), recv_endpoint_,
        [self](std::error_code ec, std::size_t length)
        {
            self->do_receive_from(ec, length);
        });
}

void CUdpClientSession::do_write(uint32 connect_id)
{
    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(session_send_buffer_.read(), session_send_buffer_.get_write_size());
    send_buffer->buffer_length_ = session_send_buffer_.get_write_size();
    session_send_buffer_.move(session_send_buffer_.get_write_size());

    send_io_data(connect_id, send_buffer);
}

void CUdpClientSession::set_write_buffer(uint32 connect_id, const char* data, size_t length)
{
    std::memcpy(session_send_buffer_.get_curr_write_ptr(),
        data,
        length);
    session_send_buffer_.set_write_data(length);
}

void CUdpClientSession::clear_write_buffer(size_t length)
{
    session_send_buffer_.move(length);
}

_ClientIPInfo CUdpClientSession::get_remote_ip(uint32 connect_id)
{
    _ClientIPInfo recv_ip_info;
    recv_ip_info.m_strClientIP = socket_.remote_endpoint().address().to_string();
    recv_ip_info.m_u2Port = socket_.remote_endpoint().port();
    return recv_ip_info;
}

void CUdpClientSession::add_send_finish_size(uint32 connect_id, size_t send_length)
{
    send_data_size_ += send_length;
}

void CUdpClientSession::do_write_immediately(uint32 connect_id, const char* data, size_t length)
{
    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(data, length);
    send_buffer->buffer_length_ = length;
    session_send_buffer_.move(session_send_buffer_.get_write_size());

    send_io_data(connect_id, send_buffer);
}

EM_CONNECT_IO_TYPE CUdpClientSession::get_io_type()
{
    return io_type_;
}

std::chrono::steady_clock::time_point& CUdpClientSession::get_recv_time(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return recv_data_time_;
}

bool CUdpClientSession::format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message)
{
    return packet_parse_interface_->parse_format_send_buffer_ptr_(connect_id, message, format_message, get_io_type());
}

bool CUdpClientSession::is_need_send_format()
{
    return packet_parse_interface_->is_need_send_format_ptr_();
}

void CUdpClientSession::send_io_data(uint32 connect_id, std::shared_ptr<CSendBuffer> send_buffer)
{
    clear_write_buffer(send_buffer->buffer_length_);

    //异步发送
    auto self(shared_from_this());

    io_context_->dispatch([self, connect_id, send_buffer]()
        {
            self->socket_.async_send_to(asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_), self->send_endpoint_,
                [self, send_buffer, connect_id](std::error_code ec, std::size_t length)
                {
                    if (ec)
                    {
                        //暂时不处理
                        PSS_LOGGER_DEBUG("[CUdpClientSession::do_write]connect_id={0}, write error({1}).", connect_id, ec.message());
                    }
                    else
                    {
                        self->add_send_finish_size(connect_id, length);
                    }
                });
        });
}

uint32 CUdpClientSession::get_mark_id(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return server_id_;
}

void CUdpClientSession::set_io_bridge_connect_id(uint32 from_io_connect_id, uint32 to_io_connect_id)
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

void CUdpClientSession::do_receive_from(std::error_code ec, std::size_t length)
{
    if (!ec)
    {
        recv_data_size_ += length;
        session_recv_buffer_.set_write_data(length);
        PSS_LOGGER_DEBUG("[CUdpClientSession::do_write]recv length={}.", length);

        std::memcpy(session_send_buffer_.get_curr_write_ptr(),
            session_recv_buffer_.read(),
            length);
        session_send_buffer_.set_write_data(length);

        //判断是否有桥接
        if (EM_SESSION_STATE::SESSION_IO_BRIDGE == io_state_)
        {
            recv_data_time_ = std::chrono::steady_clock::now();
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
                session_recv_buffer_.move(length);
                App_WorkThreadLogic::instance()->close_session_event(connect_id_);
            }
            else
            {
                recv_data_time_ = std::chrono::steady_clock::now();

                //添加到数据队列处理
                App_WorkThreadLogic::instance()->assignation_thread_module_logic(connect_id_, message_list, shared_from_this());
            }

        }

        //继续读数据
        do_receive();
    }
    else
    {
        //链接断开
        App_WorkThreadLogic::instance()->close_session_event(connect_id_);
    }
}

