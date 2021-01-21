#include "UdpSession.h"

CUdpSession::CUdpSession(asio::io_context& io_context)
    : socket_(io_context)
{
}

void CUdpSession::start(uint32 connect_id, uint32 buffer_size, string server_ip, uint16 server_port)
{
    connect_id_ = connect_id;

    session_recv_buffer_.Init(buffer_size);
    session_send_buffer_.Init(buffer_size);

    //建立连接
    udp::endpoint end_point(asio::ip::address::from_string(server_ip.c_str()), server_port);
    send_endpoint_ = end_point;
    asio::error_code connect_error;
    socket_.connect(end_point, connect_error);

    if (connect_error)
    {
        //连接建立失败
        PSS_LOGGER_DEBUG("[CUdpSession::start]error({})", connect_error.message());
    }
    else
    {
        //处理链接建立消息
        _ClientIPInfo remote_ip;
        _ClientIPInfo local_ip;
        remote_ip.m_strClientIP = socket_.remote_endpoint().address().to_string();
        remote_ip.m_u2Port = socket_.remote_endpoint().port();
        local_ip.m_strClientIP = socket_.local_endpoint().address().to_string();
        local_ip.m_u2Port = socket_.local_endpoint().port();

        PSS_LOGGER_DEBUG("[CUdpSession::start]remote({0}:{1})", remote_ip.m_strClientIP, remote_ip.m_u2Port);
        PSS_LOGGER_DEBUG("[CUdpSession::start]local({0}:{1})", local_ip.m_strClientIP, local_ip.m_u2Port);

        do_read();
    }
}

void CUdpSession::Close()
{
    socket_.close();

    //输出接收发送字节数
    PSS_LOGGER_DEBUG("[CUdpSession::Close]recv:{0}, send:{1}", recv_data_size_, send_data_size_);
}


void CUdpSession::do_read()
{
    //接收数据
    auto self(shared_from_this());

    //如果缓冲已满，断开连接，不再接受数据。
    if (session_recv_buffer_.get_buffer_size() == 0)
    {
        //链接断开(缓冲撑满了)
        App_tms::instance()->AddMessage(1, [self]() {
            self->Close();
            });
    }

    socket_.async_receive_from(asio::buffer(session_recv_buffer_.get_curr_write_ptr(), session_recv_buffer_.get_buffer_size()), recv_endpoint_,
        [this, self](std::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                recv_data_size_ += length;
                session_recv_buffer_.set_write_data(length);
                PSS_LOGGER_DEBUG("[CUdpSession::do_write]recv length={}.", length);

                std::memcpy(session_send_buffer_.get_curr_write_ptr(),
                    session_recv_buffer_.read(),
                    length);
                session_send_buffer_.set_write_data(length);

                //处理数据拆包


                session_recv_buffer_.move(length);
                //继续读数据
                self->do_read();
            }
            else
            {
                //链接断开
                Close();
            }
        });
}

void CUdpSession::do_write()
{
    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(session_send_buffer_.read(), session_send_buffer_.get_write_size());
    send_buffer->buffer_length_ = session_send_buffer_.get_write_size();
    session_send_buffer_.move(session_send_buffer_.get_write_size());

    //PSS_LOGGER_DEBUG("[CUdpSession::do_write]send_buffer->buffer_length_={}.", send_buffer->buffer_length_);

    clear_write_buffer(send_buffer->buffer_length_);

    //异步发送
    auto self(shared_from_this());
    socket_.async_send_to(asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_), send_endpoint_,
        [self, send_buffer](std::error_code ec, std::size_t length)
        {
            if (ec)
            {
                //暂时不处理
                std::cout << "[CUdpSession::do_write](" << ec.value() << ") mseeage(" << ec.message() << ")" << std::endl;
            }
            else
            {
                self->add_send_finish_size(length);
            }
        });
}

void CUdpSession::set_write_buffer(const char* data, size_t length)
{
    std::memcpy(session_send_buffer_.get_curr_write_ptr(),
        data,
        length);
    session_send_buffer_.set_write_data(length);
}

void CUdpSession::clear_write_buffer(size_t length)
{
    session_send_buffer_.move(length);
}

void CUdpSession::add_send_finish_size(size_t send_length)
{
    send_data_size_ += send_length;
}
