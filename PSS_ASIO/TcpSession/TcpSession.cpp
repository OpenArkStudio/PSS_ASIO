﻿#include "TcpSession.h"

CTcpSession::CTcpSession(tcp::socket socket, asio::io_context* io_context)
    : socket_(std::move(socket)), io_context_(io_context)
{
    // 设置UDP缓冲区大小为TCP_BUFFER_SIZE字节
    asio::socket_base::receive_buffer_size recvoption(TCP_BUFFER_SIZE);
    asio::socket_base::send_buffer_size sendoption(TCP_BUFFER_SIZE);

    std::error_code ec;
    socket_.set_option(recvoption, ec);
    if (ec) 
    {
        PSS_LOGGER_ERROR("[CTcpSession::CTcpSession]Failed to set receive buffer size: {0}", ec.message());
    }

    socket_.set_option(sendoption, ec);
    if (ec) 
    {
        PSS_LOGGER_ERROR("[CTcpSession::CTcpSession]Failed to set send buffer size: {0}", ec.message());
    }
}

void CTcpSession::open(uint32 packet_parse_id, uint32 recv_size)
{
    connect_id_ = App_ConnectCounter::instance()->CreateCounter();

    packet_parse_interface_ = App_PacketParseLoader::instance()->GetPacketParseInfo(packet_parse_id);

    session_recv_buffer_.Init(recv_size);

    recv_data_time_ = std::chrono::steady_clock::now();

    //处理链接建立消息
    remote_ip_.m_strClientIP = socket_.remote_endpoint().address().to_string();
    remote_ip_.m_u2Port = socket_.remote_endpoint().port();
    local_ip_.m_strClientIP = socket_.local_endpoint().address().to_string();
    local_ip_.m_u2Port = socket_.local_endpoint().port();
    packet_parse_interface_->packet_connect_ptr_(connect_id_, remote_ip_, local_ip_, io_type_, App_IoBridge::instance());

    //加入session 映射
    App_WorkThreadLogic::instance()->add_thread_session(connect_id_, shared_from_this(), local_ip_, remote_ip_);

#ifdef GCOV_TEST
    PSS_LOGGER_DEBUG("[CTcpSession::open]<test>****connect_id={0}***", connect_id_);
    
    //测试发送写入失败回调消息
    if (connect_id_ == 5)
    {
        std::string write_fail_text = "test write fail";
        send_write_fail_to_logic(write_fail_text, write_fail_text.length());

        //测试发送点对点链接失败消息
        vector<std::shared_ptr<CMessage_Packet>> message_list;

        App_WorkThreadLogic::instance()->assignation_thread_module_logic_iotoio_error(
            connect_id_,
            message_list,
            shared_from_this());

        //测试设置点对点连接
        set_io_bridge_connect_id(100, 101);
        set_io_bridge_connect_id(0, 0);

        PSS_LOGGER_DEBUG("[CTcpSession::open]<test>connect_id={0} is get", get_connect_id());
        get_recv_time();

        //测试格式化数据
        auto packet_format_before = std::make_shared<CMessage_Packet>();
        auto packet_format_behand = std::make_shared<CMessage_Packet>();
        format_send_packet(4, packet_format_before, packet_format_behand);
    }
#endif

    do_read();
}

_ClientIPInfo CTcpSession::get_remote_ip(uint32 connect_id)
{
    return remote_ip_;
}

void CTcpSession::close(uint32 connect_id)
{
    //如果缓冲中不存在等待发送的数据，则直接关闭
    if (send_buffer_size_ == send_data_size_)
    {
        close_immediaterly();
    }
    else
    {
        //如果缓冲区有数据，直接发送
        do_write(connect_id_);
        is_active_close_ = true;
    }
}

void CTcpSession::close_immediaterly()
{
    if (!socket_.is_open())
    {
        return;
    }
    auto self(shared_from_this());

    auto recv_data_size = recv_data_size_;
    auto send_data_size = send_data_size_;
    auto io_send_count = io_send_count_;

    EM_CONNECT_IO_TYPE io_type = io_type_;
    _ClientIPInfo remote_ip = remote_ip_;

    //关闭链接放到IO收发线程里去做
    io_context_->dispatch([self, io_type, remote_ip, recv_data_size, send_data_size, io_send_count]()
        {
            //输出接收发送字节数
            PSS_LOGGER_DEBUG("[CTcpSession::close]connect_id={0}, recv:{1}, send:{2} io_send_count:{3}",
                self->connect_id_, recv_data_size, send_data_size, io_send_count);

            //断开连接
            self->packet_parse_interface_->packet_disconnect_ptr_(self->connect_id_, io_type, App_IoBridge::instance());

            App_WorkThreadLogic::instance()->delete_thread_session(self->connect_id_, 
                self,
                remote_ip,
                io_type);
            self->socket_.close();
        });
}

void CTcpSession::do_read()
{
    //接收数据
    //如果缓冲已满，断开连接，不再接受数据。
    if (session_recv_buffer_.get_buffer_size() == 0)
    {
        //链接断开(缓冲撑满了)
        App_WorkThreadLogic::instance()->close_session_event(connect_id_, shared_from_this());
    }

    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(session_recv_buffer_.get_curr_write_ptr(), session_recv_buffer_.get_buffer_size()),
        [self](std::error_code ec, std::size_t length)
        {
            self->do_read_some(ec, length);
        });
}

void CTcpSession::do_write(uint32 connect_id)
{
    std::lock_guard<std::mutex> lck(send_thread_mutex_);

    if (session_send_buffer_.empty()) 
    {
        return;
    }

    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_ = session_send_buffer_;
    send_buffer->buffer_length_ = session_send_buffer_.size();

    //记录放入缓存的大小
    send_buffer_size_+= session_send_buffer_.size();

    clear_write_buffer();

    //如果此链接已经主动关闭，不在接受之后的发送请求
    if (is_active_close_)
    {
        send_write_fail_to_logic(send_buffer->data_, send_buffer->buffer_length_);
        return;
    }

    //异步发送
    auto self(shared_from_this());
    io_context_->dispatch([self, send_buffer, connect_id]() {
        asio::async_write(self->socket_, asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_),
            [self, send_buffer, connect_id](std::error_code ec, std::size_t length)
            {
                self->do_write_finish(ec, connect_id, send_buffer, length);
            });
    });
}

void CTcpSession::do_write_finish(const std::error_code& ec, uint32 connect_id, std::shared_ptr<CSendBuffer> send_buffer, std::size_t length)
{
    if (ec)
    {
        //发送写入消息失败信息
        PSS_LOGGER_DEBUG("[CTcpSession::do_write]({0})write error({1}).", connect_id, ec.message());
        send_write_fail_to_logic(send_buffer->data_, length);
        if (true == is_active_close_)
        {
            close_immediaterly();
        }
    }
    else
    {
        add_send_finish_size(connect_id, length);
        if (true == is_active_close_
            && send_buffer_size_ == send_data_size_)
        {
            //关闭客户端
            close_immediaterly();
        }
    }
}

void CTcpSession::do_write_immediately(uint32 connect_id, const char* data, size_t length)
{
    set_write_buffer(connect_id, data, length);
    do_write(connect_id);
}

void CTcpSession::set_write_buffer(uint32 connect_id, const char* data, size_t length)
{
    std::lock_guard<std::mutex> lck(send_thread_mutex_);

    session_send_buffer_.append(data, length);
}

void CTcpSession::clear_write_buffer()
{
    session_send_buffer_.clear();
}

void CTcpSession::do_read_some(std::error_code ec, std::size_t length)
{
    if (!ec)
    {
        recv_data_size_ += length;
        session_recv_buffer_.set_write_data(length);

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
                App_WorkThreadLogic::instance()->close_session_event(connect_id_, shared_from_this());
            }
            else
            {
                //更新接收数据时间
                recv_data_time_ = std::chrono::steady_clock::now();

                //添加消息处理
                App_WorkThreadLogic::instance()->assignation_thread_module_logic(connect_id_, message_list, shared_from_this());
            }
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

void CTcpSession::send_write_fail_to_logic(const std::string& write_fail_buffer, std::size_t buffer_length)
{
    vector<std::shared_ptr<CMessage_Packet>> message_tcp_recv_list;
    auto tcp_write_fail_packet = std::make_shared<CMessage_Packet>();
    tcp_write_fail_packet->command_id_ = LOGIC_THREAD_WRITE_IO_ERROR;
    tcp_write_fail_packet->buffer_.append(write_fail_buffer.c_str(), buffer_length);
    message_tcp_recv_list.emplace_back(tcp_write_fail_packet);

    //写IO失败消息提交给逻辑插件
    App_WorkThreadLogic::instance()->assignation_thread_module_logic_with_events(connect_id_, 
        message_tcp_recv_list, 
        shared_from_this());
}

void CTcpSession::add_send_finish_size(uint32 connect_id, size_t send_length)
{
    std::lock_guard<std::mutex> lck(send_thread_mutex_);

    send_data_size_ += send_length;

    io_send_count_++;
}

EM_CONNECT_IO_TYPE CTcpSession::get_io_type()
{
    return io_type_;
}

uint32 CTcpSession::get_mark_id(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return 0;
}

uint32 CTcpSession::get_connect_id() 
{
    return connect_id_;
}

std::chrono::steady_clock::time_point& CTcpSession::get_recv_time(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return recv_data_time_;
}

bool CTcpSession::format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message)
{
    return packet_parse_interface_->parse_format_send_buffer_ptr_(connect_id, message, format_message, get_io_type());
}

bool CTcpSession::is_need_send_format()
{
    return packet_parse_interface_->is_need_send_format_ptr_();
}

void CTcpSession::set_io_bridge_connect_id(uint32 from_io_connect_id, uint32 to_io_connect_id)
{
    if (to_io_connect_id > 0)
    {
        io_state_ = EM_SESSION_STATE::SESSION_IO_BRIDGE;
        io_bridge_connect_id_ = to_io_connect_id;
        PSS_LOGGER_DEBUG("[CTcpSession::set_io_bridge_connect_id]connect_id={}, io_bridge_connect_id:{},the bridge is set successfully.",
            connect_id_, io_bridge_connect_id_);
    }
    else
    {
        io_state_ = EM_SESSION_STATE::SESSION_IO_LOGIC;
        io_bridge_connect_id_ = 0;
    }
}

