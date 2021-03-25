#include "UdpServer.h"

CUdpServer::CUdpServer(asio::io_context& io_context, const std::string& server_ip, short port, uint32 packet_parse_id, uint32 max_recv_size, uint32 max_send_size)
    : socket_(io_context, udp::endpoint(asio::ip::address_v4::from_string(server_ip), port)), max_recv_size_(max_recv_size), max_send_size_(max_send_size)
{
    //处理链接建立消息
    PSS_LOGGER_DEBUG("[CUdpServer::do_accept]{0}:{1} Begin Accept.", server_ip, port);

    session_recv_buffer_.Init(max_recv_size_);

    packet_parse_interface_ = App_PacketParseLoader::instance()->GetPacketParseInfo(packet_parse_id);

    do_receive();
}

void CUdpServer::do_receive()
{
    socket_.async_receive_from(
        asio::buffer(session_recv_buffer_.get_curr_write_ptr(), session_recv_buffer_.get_buffer_size()), recv_endpoint_,
        [this](std::error_code ec, std::size_t length)
        {
            do_receive_from(ec, length);
        });
}

void CUdpServer::do_receive_from(std::error_code ec, std::size_t length)
{
    //查询当前的connect_id
    auto connect_id = add_udp_endpoint(recv_endpoint_, length, max_send_size_);

    if (!ec && length > 0)
    {
        //处理数据包
        auto self(shared_from_this());

        //如果缓冲已满，断开连接，不再接受数据。
        if (session_recv_buffer_.get_buffer_size() == 0)
        {
            //链接断开(缓冲撑满了)
            session_recv_buffer_.move(length);
            App_WorkThreadLogic::instance()->close_session_event(connect_id);
            do_receive();
        }

        session_recv_buffer_.set_write_data(length);

        //处理数据拆包
        vector<std::shared_ptr<CMessage_Packet>> message_list;
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
            recv_data_time_ = std::chrono::steady_clock::now();
            //添加到数据队列处理
            App_WorkThreadLogic::instance()->assignation_thread_module_logic(connect_id, message_list, self);
        }


        do_receive();
    }
    else
    {
        do_receive();
    }
}

void CUdpServer::close(uint32 connect_id)
{
    close_udp_endpoint_by_id(connect_id);
}

void CUdpServer::set_write_buffer(uint32 connect_id, const char* data, size_t length)
{
    auto session_info = find_udp_endpoint_by_id(connect_id);

    if (session_info == nullptr || session_info->session_send_buffer_.get_buffer_size() <= length)
    {
        //发送些缓冲已经满了
        PSS_LOGGER_DEBUG("[CUdpServer::set_write_buffer]({})session_info is null or session_send_buffer_ is full", connect_id);
        return;
    }

    std::memcpy(session_info->session_send_buffer_.get_curr_write_ptr(),
        data,
        length);
    session_info->session_send_buffer_.set_write_data(length);
}

void CUdpServer::clear_write_buffer(shared_ptr<CUdp_Session_Info> session_info) const
{
    session_info->session_send_buffer_.move(session_info->session_send_buffer_.get_write_size());
}

void CUdpServer::do_write(uint32 connect_id)
{
    auto session_info = find_udp_endpoint_by_id(connect_id);

    if (session_info == nullptr)
    {
        PSS_LOGGER_DEBUG("[CUdpServer::do_write]({}) is nullptr.", connect_id);
        return;
    }

    if (session_info->udp_state == EM_UDP_VALID::UDP_INVALUD)
    {
        clear_write_buffer(session_info);
        return;
    }

    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(session_info->session_send_buffer_.read(), session_info->session_send_buffer_.get_write_size());
    send_buffer->buffer_length_ = session_info->session_send_buffer_.get_write_size();

    clear_write_buffer(session_info);

    auto self(shared_from_this());
    socket_.async_send_to(
        asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_), session_info->send_endpoint,
        [self, send_buffer, connect_id](std::error_code ec, std::size_t send_length)
        {
            if (ec)
            {
                //暂时不处理
                PSS_LOGGER_DEBUG("[CUdpServer::do_write]connect_id={0}, write error({1}).", connect_id, ec.message());
            }
            else
            {
                //这里记录发送字节数
                self->add_send_finish_size(connect_id, send_length);
            }
        });
}

void CUdpServer::do_write_immediately(uint32 connect_id, const char* data, size_t length)
{
    auto session_info = find_udp_endpoint_by_id(connect_id);

    if (session_info == nullptr)
    {
        PSS_LOGGER_DEBUG("[CUdpServer::do_write]({}) is nullptr.", connect_id);
        return;
    }

    if (session_info->udp_state == EM_UDP_VALID::UDP_INVALUD)
    {
        clear_write_buffer(session_info);
        return;
    }

    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(data, length);
    send_buffer->buffer_length_ = length;

    clear_write_buffer(session_info);

    auto self(shared_from_this());
    socket_.async_send_to(
        asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_), session_info->send_endpoint,
        [self, send_buffer, connect_id](std::error_code ec, std::size_t send_length)
        {
            if (ec)
            {
                //暂时不处理
                PSS_LOGGER_DEBUG("[CUdpServer::do_write_immediately]write error({0}).", ec.message());
            }
            else
            {
                //这里记录发送字节数
                self->add_send_finish_size(connect_id, send_length);
            }
        });
}

uint32 CUdpServer::add_udp_endpoint(const udp::endpoint& recv_endpoint, size_t length, uint32 max_buffer_length)
{
    auto f = udp_endpoint_2_id_list_.find(recv_endpoint);
    if (f != udp_endpoint_2_id_list_.end())
    {
        //找到了，返回ID
        return f->second;
    }
    else
    {
        //生成一个新的ID
        auto connect_id = App_ConnectCounter::instance()->CreateCounter();

        auto session_info = make_shared<CUdp_Session_Info>();
        session_info->send_endpoint = recv_endpoint;
        session_info->recv_data_size_ += length;
        session_info->udp_state = EM_UDP_VALID::UDP_VALUD;
        session_info->session_send_buffer_.Init(max_buffer_length);

        udp_endpoint_2_id_list_[recv_endpoint] = connect_id;
        udp_id_2_endpoint_list_[connect_id] = session_info;

        //调用packet parse 链接建立
        _ClientIPInfo remote_ip;
        _ClientIPInfo local_ip;
        remote_ip.m_strClientIP = recv_endpoint.address().to_string();
        remote_ip.m_u2Port = recv_endpoint.port();
        local_ip.m_strClientIP = socket_.local_endpoint().address().to_string();
        local_ip.m_u2Port = socket_.local_endpoint().port();
        packet_parse_interface_->packet_connect_ptr_(connect_id, remote_ip, local_ip, io_type_);

        //添加映射关系
        App_WorkThreadLogic::instance()->add_thread_session(connect_id, shared_from_this(), local_ip, remote_ip);

        return connect_id;
    }
}

shared_ptr<CUdp_Session_Info> CUdpServer::find_udp_endpoint_by_id(uint32 connect_id)
{
    auto f = udp_id_2_endpoint_list_.find(connect_id);
    if (f != udp_id_2_endpoint_list_.end())
    {
        return f->second;
    }
    
    return nullptr;
}

void CUdpServer::close_udp_endpoint_by_id(uint32 connect_id)
{
    auto self(shared_from_this());

    auto f = udp_id_2_endpoint_list_.find(connect_id);
    if (f != udp_id_2_endpoint_list_.end())
    {
        //调用packet parse 断开消息
        packet_parse_interface_->packet_disconnect_ptr_(connect_id, io_type_);

        auto session_endpoint = f->second->send_endpoint;
        udp_id_2_endpoint_list_.erase(f);
        udp_endpoint_2_id_list_.erase(session_endpoint);
    }

    //删除映射关系、
    _ClientIPInfo remote_ip;
    auto end_f = udp_id_2_endpoint_list_.find(connect_id);
    if (end_f != udp_id_2_endpoint_list_.end())
    {
        remote_ip.m_strClientIP = end_f->second->send_endpoint.address().to_string();
        remote_ip.m_u2Port = end_f->second->send_endpoint.port();
    }

    App_WorkThreadLogic::instance()->delete_thread_session(connect_id, remote_ip, self);
}

void CUdpServer::add_send_finish_size(uint32 connect_id, size_t length)
{
    auto f = udp_id_2_endpoint_list_.find(connect_id);
    if (f != udp_id_2_endpoint_list_.end())
    {
        f->second->send_data_size_ += length;
    }
}

EM_CONNECT_IO_TYPE CUdpServer::get_io_type()
{
    return io_type_;
}

uint32 CUdpServer::get_mark_id(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return 0;
}

std::chrono::steady_clock::time_point& CUdpServer::get_recv_time()
{
    return recv_data_time_;
}

bool CUdpServer::format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message)
{
    return packet_parse_interface_->parse_format_send_buffer_ptr_(connect_id, message, get_io_type());
}

