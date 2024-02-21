#include "TtyServer.h"

CTTyServer::CTTyServer(uint32 packet_parse_id, uint32 max_recv_size, uint32 max_send_size, CIo_List_Manager* io_list_manager)
{
    //处理链接建立消息
    session_recv_buffer_.Init(max_recv_size);
    session_send_buffer_.Init(max_send_size);

    io_list_manager_ = io_list_manager;

    connect_id_ = App_ConnectCounter::instance()->CreateCounter();

    packet_parse_interface_ = App_PacketParseLoader::instance()->GetPacketParseInfo(packet_parse_id);
}

void CTTyServer::start(asio::io_context* io_context, const std::string& tty_name, uint16 tty_port, uint8 char_size, uint32 server_id)
{
    io_context_ = io_context;
    if (false == add_serial_port(io_context, tty_name, tty_port, char_size))
    {
        return;
    }

    recv_data_time_ = std::chrono::steady_clock::now();

    server_id_ = server_id;
    tty_name_ = tty_name;
    tty_port_ = tty_port;

    asio::serial_port::baud_rate option;
    std::error_code ec;
    serial_port_param_->get_option(option, ec);
    if (ec)
    {
        PSS_LOGGER_DEBUG("[CTTyServer::start]connect error={}.", ec.message());
    }
    else
    {
        local_ip_.m_strClientIP = "tty";
        remote_ip_.m_strClientIP = tty_name_;
        remote_ip_.m_u2Port = (uint16)option.value();

        App_WorkThreadLogic::instance()->add_thread_session(connect_id_, shared_from_this(), local_ip_, remote_ip_);

        packet_parse_interface_->packet_connect_ptr_(connect_id_, remote_ip_, local_ip_, io_type_, App_IoBridge::instance());

        //添加点对点映射
        if (true == App_IoBridge::instance()->regedit_bridge_session_id(remote_ip_, io_type_, connect_id_))
        {
            io_state_ = EM_SESSION_STATE::SESSION_IO_BRIDGE;
        }

        //查看这个链接是否有桥接信息
    //查看这个链接是否有桥接信息
        auto io_bridge_result = App_IoBridge::instance()->get_to_session_id(connect_id_, remote_ip_);

        //对不同类型进行点对点透传进行单独判定
        if (io_bridge_result.io_bridge_id_ > 0
            && io_bridge_result.bridge_type_ == ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH)
        {
            io_bridge_connect_id_ = io_bridge_result.io_bridge_id_;

            PSS_LOGGER_DEBUG("[CTTyServer::start]connect_id={} IO_BRIDGE_BATH, io_bridge_connect_id:{},the bridge is set successfully.",
                connect_id_, io_bridge_connect_id_);

            //如果桥接成立，设置对端的桥接地址
            App_WorkThreadLogic::instance()->set_io_bridge_connect_id(io_bridge_connect_id_,
                connect_id_);
        }
        else if (io_bridge_result.io_bridge_id_ > 0
            && io_bridge_result.bridge_type_ == ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_FROM)
        {
            //如果是单向的，看看朝向
            if (io_bridge_result.io_bridge_id_ == io_bridge_result.io_from_id_)
            {
                //如果是相同朝向的，设置自己的对端透传ID
                io_bridge_connect_id_ = io_bridge_result.io_bridge_id_;

                PSS_LOGGER_DEBUG("[CTTyServer::start]connect_id={} ID from, io_bridge_connect_id:{},the bridge is set successfully.",
                    connect_id_, io_bridge_connect_id_);
            }
            else
            {
                //如果是对端,当前链接建立后，需要设置对端可以透传。
                io_bridge_connect_id_ = 0;

                //如果桥接成立，设置对端的桥接地址
                App_WorkThreadLogic::instance()->set_io_bridge_connect_id(io_bridge_result.io_from_id_,
                    io_bridge_result.io_to_id_);

                PSS_LOGGER_DEBUG("[CTTyServer::start]connect_id={} ID to, io_bridge_connect_id:{},the bridge is set successfully.",
                    connect_id_, io_bridge_connect_id_);
            }
        }

        io_list_manager_->add_accept_net_io_event(tty_name_, tty_port_, EM_CONNECT_IO_TYPE::CONNECT_IO_TTY, std::dynamic_pointer_cast<CIo_Net_server>(shared_from_this()));

        do_receive();
    }
}

_ClientIPInfo CTTyServer::get_remote_ip(uint32 connect_id)
{
    return remote_ip_;
}

void CTTyServer::do_receive()
{
    auto self(shared_from_this());
    serial_port_param_->async_read_some(asio::buffer(session_recv_buffer_.get_curr_write_ptr(), session_recv_buffer_.get_buffer_size()),
        [self](std::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                //处理接收数据
                self->do_read_some(ec, length);
            }
            else
            {
                PSS_LOGGER_DEBUG("[CTTyServer::do_receive]({}:{})accept error:{}.", 
                    self->tty_name_, 
                    self->tty_port_, 
                    ec.message());

                App_WorkThreadLogic::instance()->add_frame_events(LOGIC_LISTEN_SERVER_ERROR,
                    self->server_id_,
                    self->tty_name_,
                    self->tty_port_,
                    EM_CONNECT_IO_TYPE::CONNECT_IO_TTY);

                return;
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

bool CTTyServer::add_serial_port(asio::io_context* io_context, const std::string& tty_name, uint16 tty_port, uint8 char_size)
{
    std::error_code ec;
    serial_port_param_ = std::make_shared<asio::serial_port>(*io_context);

    serial_port_param_->open(tty_name, ec);

    if (ec)
    {
        PSS_LOGGER_DEBUG("[CServerService::add_serial_port]connect error={}.", ec.message());

        //发送消息给逻辑块
        App_WorkThreadLogic::instance()->add_frame_events(LOGIC_LISTEN_SERVER_ERROR,
            server_id_,
            local_ip_.m_strClientIP,
            local_ip_.m_u2Port,
            io_type_);

        return false;
    }

    serial_port_param_->set_option(asio::serial_port::baud_rate(tty_port), ec);
    serial_port_param_->set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none), ec);
    serial_port_param_->set_option(asio::serial_port::parity(asio::serial_port::parity::none), ec);
    serial_port_param_->set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one), ec);
    serial_port_param_->set_option(asio::serial_port::character_size(char_size), ec);

    return true;
}

void CTTyServer::do_write(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);

    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(session_send_buffer_.read(), session_send_buffer_.get_write_size());
    send_buffer->buffer_length_ = session_send_buffer_.get_write_size();

    //记录放入缓存的大小
    send_buffer_size_ += send_buffer->buffer_length_;

    clear_write_buffer();
    
    //异步发送
    auto self(shared_from_this());
    serial_port_param_->async_write_some(asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_),
        [self, send_buffer, connect_id](std::error_code ec, std::size_t length)
        {
            self->do_write_finish(ec, connect_id, send_buffer, length);
        });

    clear_write_buffer();
}

void CTTyServer::do_write_finish(std::error_code& ec, uint32 connect_id, std::shared_ptr<CSendBuffer> send_buffer, std::size_t length)
{
    if (ec)
    {
        //回调消息处理失败信息
        PSS_LOGGER_DEBUG("[CTTyServer::do_write]({0})write error({1}).", connect_id, ec.message());

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

void CTTyServer::do_write_immediately(uint32 connect_id, const char* data, size_t length)
{
    PSS_UNUSED_ARG(connect_id);

    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(data, length);
    send_buffer->buffer_length_ = length;

    //异步发送
    auto self(shared_from_this());

    io_context_->dispatch([self, connect_id, send_buffer]()
        {
            self->serial_port_param_->async_write_some(asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_),
                [self, send_buffer, connect_id](std::error_code ec, std::size_t send_length)
                {
                    if (ec)
                    {
                        //暂时不处理
                        PSS_LOGGER_DEBUG("[CTTyServer::do_write]write error({0}).", ec.message());
                    }
                    else
                    {
                        self->add_send_finish_size(connect_id, send_length);
                    }
                });

            self->clear_write_buffer();
        });
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

void CTTyServer::close_immediaterly()
{
    auto self(shared_from_this());

    auto io_type = io_type_;
    _ClientIPInfo remote_ip = remote_ip_;

    io_context_->dispatch([self, io_type, remote_ip]()
        {
            PSS_UNUSED_ARG(self->connect_id_);
            self->packet_parse_interface_->packet_disconnect_ptr_(self->connect_id_, io_type, App_IoBridge::instance());

            //删除映射关系
            App_WorkThreadLogic::instance()->delete_thread_session(self->connect_id_, self);

            self->io_list_manager_->del_accept_net_io_event(self->tty_name_, self->tty_port_, EM_CONNECT_IO_TYPE::CONNECT_IO_TTY);
        });
}

void CTTyServer::close()
{
    close(connect_id_);
}

uint32 CTTyServer::get_mark_id(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return server_id_;
}

uint32 CTTyServer::get_connect_id() 
{
    return connect_id_;
}

void CTTyServer::regedit_bridge_session_id(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return;
}

std::chrono::steady_clock::time_point& CTTyServer::get_recv_time(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return recv_data_time_;
}

bool CTTyServer::format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message)
{
    return packet_parse_interface_->parse_format_send_buffer_ptr_(connect_id, message, format_message, get_io_type());
}

bool CTTyServer::is_need_send_format()
{
    return packet_parse_interface_->is_need_send_format_ptr_();
}

void CTTyServer::set_io_bridge_connect_id(uint32 from_io_connect_id, uint32 to_io_connect_id)
{
    if (to_io_connect_id > 0)
    {
        io_state_ = EM_SESSION_STATE::SESSION_IO_BRIDGE;
        io_bridge_connect_id_ = to_io_connect_id;
    }
    else
    {
        io_state_ = EM_SESSION_STATE::SESSION_IO_LOGIC;
        io_bridge_connect_id_ = 0;
    }
}

void CTTyServer::do_read_some(std::error_code ec, std::size_t length)
{
    auto connect_id = connect_id_;

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
            App_WorkThreadLogic::instance()->close_session_event(connect_id, shared_from_this());
            do_receive();
        }

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
                session_recv_buffer_.move(length);
                App_WorkThreadLogic::instance()->close_session_event(connect_id, shared_from_this());
                do_receive();
            }
            else
            {
                recv_data_time_ = std::chrono::steady_clock::now();

                //添加到数据队列处理
                App_WorkThreadLogic::instance()->assignation_thread_module_logic(connect_id, message_list, self);
            }
        }

        do_receive();
    }
    else
    {
        do_receive();
    }
}

void CTTyServer::send_write_fail_to_logic(const std::string& write_fail_buffer, std::size_t buffer_length)
{
    vector<std::shared_ptr<CMessage_Packet>> message_tty_list;
    auto tty_write_fail_packet = std::make_shared<CMessage_Packet>();
    tty_write_fail_packet->command_id_ = LOGIC_THREAD_WRITE_IO_ERROR;
    tty_write_fail_packet->buffer_.append(write_fail_buffer.c_str(), buffer_length);
    message_tty_list.emplace_back(tty_write_fail_packet);

    //写IO失败消息提交给逻辑插件
    App_WorkThreadLogic::instance()->assignation_thread_module_logic(connect_id_, message_tty_list, shared_from_this());
}

