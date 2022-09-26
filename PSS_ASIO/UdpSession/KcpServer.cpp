#include "KcpServer.h"

CKcpServer::CKcpServer(asio::io_context& io_context, const std::string& server_ip, short port, uint32 packet_parse_id, uint32 max_recv_size, uint32 max_send_size, uint32 kcp_id)
    : socket_(io_context, udp::endpoint(asio::ip::address_v4::from_string(server_ip), port)), max_recv_size_(max_recv_size), max_send_size_(max_send_size), io_context_(&io_context), kcp_id_(kcp_id)
{
    //处理链接建立消息
    PSS_LOGGER_DEBUG("[CKcpServer::do_accept]{0}:{1} Begin Accept.", server_ip, port);

    session_recv_buffer_.Init(max_recv_size_);
    session_recv_data_buffer_.Init(max_recv_size_);

    packet_parse_interface_ = App_PacketParseLoader::instance()->GetPacketParseInfo(packet_parse_id);

    //建立kcp
    kcpcb_ = ikcp_create(kcp_id_, this);
    if (nullptr == kcpcb_)
    {
        PSS_LOGGER_DEBUG("[CKcpServer::do_accept]{0}:{1} kcp is null.", server_ip, port);
    }
    else
    {
        //绑定回调函数
        kcpcb_->output = kcp_udpOutPut;

        ikcp_nodelay(kcpcb_, 0, 10, 0, 0);
        ikcp_wndsize(kcpcb_, max_send_size, max_recv_size);
    }
}

void CKcpServer::start()
{
    do_receive();
}

void CKcpServer::do_receive()
{
    auto self(shared_from_this());
    socket_.async_receive_from(
        asio::buffer(session_recv_buffer_.get_curr_write_ptr(), session_recv_buffer_.get_buffer_size()), recv_endpoint_,
        [self](std::error_code ec, std::size_t length)
        {
            self->do_receive_from(ec, length);
        });
}

void CKcpServer::do_receive_from(std::error_code ec, std::size_t length)
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
            return;
        }

        session_recv_buffer_.set_write_data(length);

        //处理kcp的数据解析
        auto kcp_recv_length = ikcp_input(kcpcb_, session_recv_buffer_.read(), (long)length);
        if (kcp_recv_length < 0)//检测ikcp_input对 buf 是否提取到真正的数据	
        {
            session_recv_buffer_.set_write_data(length);
            do_receive();
            return;
        }

        //将kcp中的数据解析出来
        int logic_data_length = 0;
        while (1)
        {
            auto kcp_data_recv_length = ikcp_recv(kcpcb_, session_recv_data_buffer_.read(), (long)length);
            if (kcp_data_recv_length < 0)
            {
                break;
            }
            else
            {
                //设置写入buffer长度
                logic_data_length += kcp_data_recv_length;
                session_recv_data_buffer_.set_write_data(kcp_data_recv_length);
            }
        }

        //清理原始数据
        session_recv_buffer_.set_write_data(length);

        //处理数据拆包
        vector<std::shared_ptr<CMessage_Packet>> message_list;
        bool ret = packet_parse_interface_->packet_from_recv_buffer_ptr_(connect_client_id_, &session_recv_data_buffer_, message_list, io_type_);
        if (!ret)
        {
            //链接断开(解析包不正确)
            session_recv_data_buffer_.move(logic_data_length);
            App_WorkThreadLogic::instance()->close_session_event(connect_id);
        }
        else
        {
            recv_data_time_ = std::chrono::steady_clock::now();
            //添加到数据队列处理
            App_WorkThreadLogic::instance()->assignation_thread_module_logic(connect_id, message_list, self);
        }
    }

    //持续接收数据
    do_receive();
}

void CKcpServer::close(uint32 connect_id)
{
    auto self(shared_from_this());
    io_context_->dispatch([self, connect_id]() 
        {
            self->close_udp_endpoint_by_id(connect_id);
        });

    ikcp_release(kcpcb_); 
    kcpcb_ = nullptr;
}

void CKcpServer::set_write_buffer(uint32 connect_id, const char* data, size_t length)
{
    auto session_info = find_udp_endpoint_by_id(connect_id);

    if (session_info == nullptr || session_info->session_send_buffer_.get_buffer_size() <= length)
    {
        //发送些缓冲已经满了
        PSS_LOGGER_DEBUG("[CKcpServer::set_write_buffer]({})session_info is null or session_send_buffer_ is full", connect_id);
        return;
    }

    std::memcpy(session_info->session_send_buffer_.get_curr_write_ptr(),
        data,
        length);
    session_info->session_send_buffer_.set_write_data(length);
}

void CKcpServer::clear_write_buffer(shared_ptr<CKcp_Session_Info> session_info) const
{
    session_info->session_send_buffer_.move(session_info->session_send_buffer_.get_write_size());
}

void CKcpServer::do_write(uint32 connect_id)
{
    auto session_info = find_udp_endpoint_by_id(connect_id);

    if (session_info == nullptr)
    {
        PSS_LOGGER_DEBUG("[CKcpServer::do_write]({}) is nullptr.", connect_id);
        return;
    }

    if (session_info->udp_state == EM_KCP_VALID::KCP_INVALUD)
    {
        clear_write_buffer(session_info);
        return;
    }

    //将要处理的数据封装为kcp数据包再发送出去
    set_kcp_send_info(connect_id, session_info->send_endpoint);
    int	ret = ikcp_send(kcpcb_, session_info->session_send_buffer_.read(), (long)session_info->session_send_buffer_.get_write_size());
    if (ret != 0)
    {
        PSS_LOGGER_DEBUG("[CKcpServer::do_write]({}) send error ret={}.", connect_id, ret);
    }

    clear_write_buffer(session_info);
}

void CKcpServer::do_write_immediately(uint32 connect_id, const char* data, size_t length)
{
    auto session_info = find_udp_endpoint_by_id(connect_id);

    if (session_info == nullptr)
    {
        PSS_LOGGER_DEBUG("[CKcpServer::do_write]({}) is nullptr.", connect_id);
        return;
    }

    if (session_info->udp_state != EM_KCP_VALID::KCP_INVALUD)
    {
        set_kcp_send_info(connect_id, session_info->send_endpoint);
        int	ret = ikcp_send(kcpcb_, data, (long)length);
        if (ret != 0)
        {
            PSS_LOGGER_DEBUG("[CKcpServer::do_write]({}) send error ret={}.", connect_id, ret);
        }
    }

    clear_write_buffer(session_info);

}

uint32 CKcpServer::add_udp_endpoint(const udp::endpoint& recv_endpoint, size_t length, uint32 max_buffer_length)
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

        auto session_info = make_shared<CKcp_Session_Info>();
        session_info->send_endpoint = recv_endpoint;
        session_info->recv_data_size_ += length;
        session_info->udp_state = EM_KCP_VALID::KCP_VALUD;
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

shared_ptr<CKcp_Session_Info> CKcpServer::find_udp_endpoint_by_id(uint32 connect_id)
{
    auto f = udp_id_2_endpoint_list_.find(connect_id);
    if (f != udp_id_2_endpoint_list_.end())
    {
        return f->second;
    }
    
    return nullptr;
}

void CKcpServer::close_udp_endpoint_by_id(uint32 connect_id)
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

void CKcpServer::add_send_finish_size(uint32 connect_id, size_t length)
{
    auto f = udp_id_2_endpoint_list_.find(connect_id);
    if (f != udp_id_2_endpoint_list_.end())
    {
        f->second->send_data_size_ += length;
    }
}

void CKcpServer::send_io_data_to_point(const char* data, size_t length)
{
    uint32 connect_id = kcp_send_info_.connecy_id_;
    udp::endpoint send_endpoint = kcp_send_info_.send_endpoint;

    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(data, length);
    send_buffer->buffer_length_ = length;

    auto self(shared_from_this());
    socket_.async_send_to(
        asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_), send_endpoint,
        [self, send_buffer, connect_id](std::error_code ec, std::size_t send_length)
        {
            if (ec)
            {
                //暂时不处理
                PSS_LOGGER_DEBUG("[CKcpServer::do_write_immediately]write error({0}).", ec.message());
            }
            else
            {
                //这里记录发送字节数
                self->add_send_finish_size(connect_id, send_length);
            }
        });
}

EM_CONNECT_IO_TYPE CKcpServer::get_io_type()
{
    return io_type_;
}

uint32 CKcpServer::get_mark_id(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return 0;
}

std::chrono::steady_clock::time_point& CKcpServer::get_recv_time()
{
    return recv_data_time_;
}

bool CKcpServer::format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message)
{
    return packet_parse_interface_->parse_format_send_buffer_ptr_(connect_id, message, format_message, get_io_type());
}

bool CKcpServer::is_need_send_format()
{
    return packet_parse_interface_->is_need_send_format_ptr_();
}

void CKcpServer::set_kcp_send_info(uint32 connect_id, udp::endpoint kcp_send_endpoint)
{
    kcp_send_info_.connecy_id_   = connect_id;
    kcp_send_info_.send_endpoint = kcp_send_endpoint;
}

int kcp_udpOutPut(const char* buf, int len, ikcpcb* kcp, void* user)
{
    //发送kcp数据
    CKcpServer* kcp_server = (CKcpServer*)user;
    kcp_server->send_io_data_to_point(buf, len);
    return 0;
}
