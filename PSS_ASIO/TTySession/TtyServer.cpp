#include "TtyServer.h"

CTTyServer::CTTyServer(shared_ptr<asio::serial_port> serial_port_param, uint32 packet_parse_id, uint32 max_recv_size, uint32 max_send_size)
    : serial_port_param_(serial_port_param)
{
    //处理链接建立消息
    session_recv_buffer_.Init(max_recv_size);
    session_send_buffer_.Init(max_send_size);

    connect_client_id_ = App_ConnectCounter::instance()->CreateCounter();

    packet_parse_interface_ = App_PacketParseLoader::instance()->GetPacketParseInfo(packet_parse_id);
}

void CTTyServer::start()
{
    _ClientIPInfo local_info;
    _ClientIPInfo romote_info;

    asio::serial_port::baud_rate option;
    std::error_code ec;
    serial_port_param_->get_option(option, ec);
    if (ec)
    {
        PSS_LOGGER_DEBUG("[CTTyServer::CTTyServer]connect error={}.", ec.message());
    }
    else
    {
        local_info.m_strClientIP = "tty";
        romote_info.m_strClientIP = "tty";
        romote_info.m_u2Port = option.value();

        App_WorkThreadLogic::instance()->add_thread_session(connect_client_id_, shared_from_this(), local_info, romote_info);

        _ClientIPInfo remote_ip;
        _ClientIPInfo local_ip;
        packet_parse_interface_->packet_connect_ptr_(connect_client_id_, remote_ip, local_ip, io_type_);

        do_receive();
    }
}

void CTTyServer::do_receive()
{
    serial_port_param_->async_read_some(asio::buffer(session_recv_buffer_.get_curr_write_ptr(), session_recv_buffer_.get_buffer_size()),
        [this](std::error_code ec, std::size_t length)
        {
            auto connect_id = connect_client_id_;

            if (!ec && length > 0)
            {
                //处理数据包
                auto self(shared_from_this());

                recv_data_size_ += length;

                //如果缓冲已满，断开连接，不再接受数据。
                if (session_recv_buffer_.get_buffer_size() == 0)
                {
                    //不断开(缓冲撑满了)
                    session_recv_buffer_.move(length);
                    App_WorkThreadLogic::instance()->close_session_event(connect_id);
                    do_receive();
                }

                session_recv_buffer_.set_write_data(length);

                //处理数据拆包
                vector<CMessage_Packet> message_list;
                bool ret = packet_parse_interface_->packet_from_recv_buffer_ptr_(connect_client_id_, &session_recv_buffer_, message_list, io_type_);
                if (!ret)
                {
                    //链接断开(解析包不正确)
                    session_recv_buffer_.move(length);
                    App_WorkThreadLogic::instance()->close_session_event(connect_id);
                    do_receive();
                }
                else
                {
                    //添加到数据队列处理
                    App_WorkThreadLogic::instance()->do_thread_module_logic(connect_id, message_list, self);
                }

                do_receive();
            }
            else
            {
                do_receive();
            }
        });
}

void CTTyServer::set_write_buffer(uint32 connect_id, const char* data, size_t length)
{
    if (session_send_buffer_.get_buffer_size() <= length)
    {
        //发送些缓冲已经满了
        PSS_LOGGER_DEBUG("[CTTyServer::set_write_buffer]connect_id={0} is full.", connect_id);
        return;
    }

    std::memcpy(session_send_buffer_.get_curr_write_ptr(),
        data,
        length);
    session_send_buffer_.set_write_data(length);
}

void CTTyServer::clear_write_buffer()
{
    session_send_buffer_.move(session_send_buffer_.get_write_size());
}

void CTTyServer::do_write(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);

    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(session_send_buffer_.read(), session_send_buffer_.get_write_size());
    send_buffer->buffer_length_ = session_send_buffer_.get_write_size();

    //PSS_LOGGER_DEBUG("[CTTyServer::do_write]send_buffer->buffer_length_={}.", send_buffer->buffer_length_);
    clear_write_buffer();
    
    //异步发送
    auto self(shared_from_this());
    serial_port_param_->async_write_some(asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_),
        [self, send_buffer, connect_id](std::error_code ec, std::size_t length)
        {
            if (ec)
            {
                //暂时不处理
                PSS_LOGGER_DEBUG("[CTTyServer::do_write]write error({0}).", ec.message());
            }
            else
            {
                self->add_send_finish_size(connect_id, length);
            }
        });

    clear_write_buffer();
}

void CTTyServer::do_write_immediately(uint32 connect_id, const char* data, size_t length)
{
    PSS_UNUSED_ARG(connect_id);

    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(data, length);
    send_buffer->buffer_length_ = length;

    //PSS_LOGGER_DEBUG("[CTTyServer::do_write]send_buffer->buffer_length_={}.", send_buffer->buffer_length_);

    //异步发送
    auto self(shared_from_this());
    serial_port_param_->async_write_some(asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_),
        [self, send_buffer, connect_id](std::error_code ec, std::size_t length)
        {
            if (ec)
            {
                //暂时不处理
                PSS_LOGGER_DEBUG("[CTTyServer::do_write]write error({0}).", ec.message());
            }
            else
            {
                self->add_send_finish_size(connect_id, length);
            }
        });

    clear_write_buffer();
}

void CTTyServer::add_send_finish_size(uint32 connect_id, size_t send_length)
{
    //异步写返回
    send_data_size_ += send_length;
}

EM_CONNECT_IO_TYPE CTTyServer::get_io_type()
{
    return io_type_;
}

void CTTyServer::close(uint32 connect_id)
{
    auto self(shared_from_this());

    PSS_UNUSED_ARG(connect_id);
    packet_parse_interface_->packet_disconnect_ptr_(connect_client_id_, io_type_);

    //删除映射关系
    App_WorkThreadLogic::instance()->delete_thread_session(connect_id, self);
}

uint32 CTTyServer::get_mark_id(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return 0;
}

