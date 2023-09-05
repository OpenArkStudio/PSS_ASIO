#include "KcpServer.h"

static int kcp_udpOutPut(const char* buf, int len, ikcpcb* kcp, void* user);

CKcpServer::CKcpServer(asio::io_context* io_context, const std::string& server_ip, io_port_type port, uint32 packet_parse_id, uint32 max_recv_size, uint32 max_send_size)
    : socket_(*io_context, udp::endpoint(asio::ip::address_v4::from_string(server_ip), port)), max_recv_size_(max_recv_size), max_send_size_(max_send_size), io_context_(io_context)
{
    //处理链接建立消息
    PSS_LOGGER_DEBUG("[CKcpServer::do_accept]{0}:{1} Begin Accept.", server_ip, port);

    session_recv_buffer_.Init(max_recv_size_);
    session_recv_data_buffer_.Init(max_recv_size_);

    packet_parse_interface_ = App_PacketParseLoader::instance()->GetPacketParseInfo(packet_parse_id);

}

void CKcpServer::start()
{
    do_receive();
}

_ClientIPInfo CKcpServer::get_remote_ip(uint32 connect_id)
{
    _ClientIPInfo remote_ip_info;
    auto f = udp_id_2_endpoint_list_.find(connect_id);
    if (f != udp_id_2_endpoint_list_.end())
    {
        //找到了
        remote_ip_info.m_strClientIP = f->second->send_endpoint.address().to_string();
        remote_ip_info.m_u2Port = f->second->send_endpoint.port();
    }

    return remote_ip_info;
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

    auto session_kcp = find_udp_endpoint_by_id(connect_id);

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

        //如果是第一次链接KCP，需要协商kcp_id，然后返回给客户端，再让客户端带着kcp_id请求过来
        if (true == is_kcp_id_create(session_recv_buffer_.read(), (uint32)length))
        {
            //发送kcp_id数据
            session_recv_buffer_.move(length);
            
            char send_kcp_id[20] = { '\0' };
            auto send_kcp_id_size = (uint32)sizeof(uint32);
            std::memcpy(send_kcp_id, &connect_id, send_kcp_id_size);
            kcp_send_info_.connect_id_ = connect_id;
            send_io_data_to_point(send_kcp_id, send_kcp_id_size, recv_endpoint_);
            session_kcp->udp_state_ = EM_KCP_VALID::KCP_ID_VALID;
            do_receive();
            return;
        }

        //如果是第一次链接KCP，需要协商kcp_id，然后返回给客户端，再让客户端带着kcp_id请求过来

        kcp_mutex_.lock();
        //刷新kcp循环
        ikcp_update(session_kcp->kcpcb_, iclock());

        //处理kcp的数据解析
        ikcp_input(session_kcp->kcpcb_, session_recv_buffer_.read(), (long)length);
        
        //将kcp中的数据解析出来
        int logic_data_length = 0;
        while (true)
        {
            auto kcp_data_recv_length = ikcp_recv(session_kcp->kcpcb_, session_recv_data_buffer_.read(), (int)length);
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
        kcp_mutex_.unlock();

        //清理原始数据
        session_recv_buffer_.move(length);

        if (logic_data_length == 0)
        {
            //kcp 回应帧数据包不处理
            do_receive();
            return;
        }

        //判断是否有桥接
        if (EM_SESSION_STATE::SESSION_IO_BRIDGE == io_state_ && io_bridge_connect_id_ > 0)
        {
            //将数据转发给桥接接口
            auto bridge_packet = std::make_shared<CMessage_Packet>();
            bridge_packet->buffer_.append(session_recv_buffer_.read(), length);
            App_WorkThreadLogic::instance()->send_io_bridge_message(io_bridge_connect_id_, bridge_packet);
            session_recv_buffer_.move(length);
        }
        else
        {
            //处理数据拆包
            vector<std::shared_ptr<CMessage_Packet>> message_list;
            bool ret = packet_parse_interface_->packet_from_recv_buffer_ptr_(connect_id, &session_recv_data_buffer_, message_list, io_type_);
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

        kcp_mutex_.lock();
        //回复udp确认信息
        set_kcp_send_info(connect_id, recv_endpoint_);

        //刷新kcp循环
        ikcp_update(session_kcp->kcpcb_, iclock());
        kcp_mutex_.unlock();
    }

    //持续接收数据
    do_receive();
}

void CKcpServer::close(uint32 connect_id)
{
    if(!socket_.is_open())
    {
        return;
    }
    auto self(shared_from_this());

    io_context_->dispatch([self, connect_id]() 
        {
            //释放kcp资源
            auto session_info = self->find_udp_endpoint_by_id(connect_id);
            session_info->close_kcp();

            self->close_udp_endpoint_by_id(connect_id);
        });
}

void CKcpServer::close_all()
{
    //释放所有kcp资源
    for (const auto& session_info : udp_id_2_endpoint_list_)
    {
        session_info.second->close_kcp();
        this->close(session_info.first);
    }

    udp_id_2_endpoint_list_.clear();
    udp_endpoint_2_id_list_.clear();
    socket_.close();
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

    if (session_info->udp_state_ != EM_KCP_VALID::KCP_ID_VALID)
    {
        PSS_LOGGER_DEBUG("[CKcpServer::do_write]({}) is udp_state is KCP_INVALUD.", connect_id);
        clear_write_buffer(session_info);
        return;
    }

    kcp_mutex_.lock();
    //回复udp确认信息
    set_kcp_send_info(connect_id, session_info->send_endpoint);

    //将要处理的数据封装为kcp数据包再发送出去
    int	ret = ikcp_send(session_info->kcpcb_, session_info->session_send_buffer_.read(), (long)session_info->session_send_buffer_.get_write_size());
    if (ret != 0)
    {
        PSS_LOGGER_DEBUG("[CKcpServer::do_write]({}) send error ret={}.", connect_id, ret);
    }

    //刷新kcp循环
    ikcp_update(session_info->kcpcb_, iclock());
    kcp_mutex_.unlock();

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

    
    if (session_info->udp_state_ == EM_KCP_VALID::KCP_ID_VALID)
    {
        kcp_mutex_.lock();

        set_kcp_send_info(connect_id, session_info->send_endpoint);
        int	ret = ikcp_send(session_info->kcpcb_, data, (int)length);
        if (ret != 0)
        {
            PSS_LOGGER_DEBUG("[CKcpServer::do_write]({}) send error ret={}.", connect_id, ret);
        }


        //刷新kcp循环
        ikcp_update(session_info->kcpcb_, iclock());
        kcp_mutex_.unlock();
    }
    else
    {
        PSS_LOGGER_DEBUG("[CKcpServer::do_write](connect_id={}) session is over .", connect_id);
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
        session_info->udp_state_ = EM_KCP_VALID::KCP_VALUD;
        session_info->session_send_buffer_.Init(max_buffer_length);

        udp_endpoint_2_id_list_[recv_endpoint] = connect_id;
        udp_id_2_endpoint_list_[connect_id] = session_info;

        //创建KCP
        session_info->init_kcp(connect_id, max_recv_size_, max_send_size_, kcp_udpOutPut, this);

        //刷新一下时间
        ikcp_update(session_info->kcpcb_, iclock());

        //调用packet parse 链接建立
        _ClientIPInfo remote_ip;
        _ClientIPInfo local_ip;
        remote_ip.m_strClientIP = recv_endpoint.address().to_string();
        remote_ip.m_u2Port = recv_endpoint.port();
        local_ip.m_strClientIP = socket_.local_endpoint().address().to_string();
        local_ip.m_u2Port = socket_.local_endpoint().port();
        packet_parse_interface_->packet_connect_ptr_(connect_id, remote_ip, local_ip, io_type_, App_IoBridge::instance());

        //添加点对点映射
        if (true == App_IoBridge::instance()->regedit_bridge_session_id(remote_ip, io_type_, connect_id))
        {
            io_state_ = EM_SESSION_STATE::SESSION_IO_BRIDGE;
        }

        //查看这个链接是否有桥接信息
        io_bridge_connect_id_ = App_IoBridge::instance()->get_to_session_id(connect_id, remote_ip);
        if (io_bridge_connect_id_ > 0)
        {
            App_WorkThreadLogic::instance()->set_io_bridge_connect_id(connect_id, io_bridge_connect_id_);
        }

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

    _ClientIPInfo remote_ip;

    auto f = udp_id_2_endpoint_list_.find(connect_id);
    if (f != udp_id_2_endpoint_list_.end())
    {
        //调用packet parse 断开消息
        packet_parse_interface_->packet_disconnect_ptr_(connect_id, io_type_, App_IoBridge::instance());

        auto session_endpoint = f->second->send_endpoint;

        remote_ip.m_strClientIP = f->second->send_endpoint.address().to_string();
        remote_ip.m_u2Port = f->second->send_endpoint.port();

        udp_id_2_endpoint_list_.erase(f);
        udp_endpoint_2_id_list_.erase(session_endpoint);
    }

    //删除映射关系
    App_WorkThreadLogic::instance()->delete_thread_session(connect_id, self);
}

void CKcpServer::add_send_finish_size(uint32 connect_id, size_t length)
{
    auto f = udp_id_2_endpoint_list_.find(connect_id);
    if (f != udp_id_2_endpoint_list_.end())
    {
        f->second->send_data_size_ += length;
    }
}

void CKcpServer::send_io_data_to_point(const char* data, size_t length, udp::endpoint send_endpoint)
{
    uint32 connect_id = kcp_send_info_.connect_id_;

    if (connect_id == 0)
    {
        return;
    }

    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(data, length);
    send_buffer->buffer_length_ = length;

    //设置connect_id无效
    kcp_send_info_.connect_id_ = 0;

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

uint32 CKcpServer::get_connect_id() 
{
    return 0;
}

void CKcpServer::regedit_bridge_session_id(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return;
}

std::chrono::steady_clock::time_point& CKcpServer::get_recv_time(uint32 connect_id)
{
    PSS_UNUSED_ARG(connect_id);
    return recv_data_time_;
}

void CKcpServer::set_io_bridge_connect_id(uint32 from_io_connect_id, uint32 to_io_connect_id)
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

bool CKcpServer::format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message)
{
    return packet_parse_interface_->parse_format_send_buffer_ptr_(connect_id, message, format_message, get_io_type());
}

bool CKcpServer::is_need_send_format()
{
    return packet_parse_interface_->is_need_send_format_ptr_();
}

bool CKcpServer::is_kcp_id_create(const char* kcp_data, uint32 kcp_size) const
{
    if (kcp_size >= 24)
    {
        return false;
    }

    std::string kcp_client_data;
    kcp_client_data.append(kcp_data, kcp_size);
    if (kcp_size == kcp_key_id_create.length() && kcp_key_id_create == kcp_client_data)
    {
        return true;
    }
    else
    {
        return false;
    }
}

udp::endpoint CKcpServer::get_kcp_send_endpoint() const
{
    return kcp_send_info_.send_endpoint;
}

void CKcpServer::set_kcp_send_info(uint32 connect_id, udp::endpoint kcp_send_endpoint)
{
    kcp_send_info_.connect_id_   = connect_id;
    kcp_send_info_.send_endpoint = kcp_send_endpoint;
}

void CKcpServer::itimeofday(long* sec, long* usec)
{
#if defined(__unix)
    struct timeval time;
    gettimeofday(&time, NULL);
    if (sec) *sec = time.tv_sec;
    if (usec) *usec = time.tv_usec;
#else
    static long mode = 0, addsec = 0;
    BOOL retval;
    static IINT64 freq = 1;
    IINT64 qpc;
    if (mode == 0) {
        retval = QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
        freq = (freq == 0) ? 1 : freq;
        retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
        addsec = (long)time(NULL);
        addsec = addsec - (long)((qpc / freq) & 0x7fffffff);
        mode = 1;
    }
    retval = QueryPerformanceCounter((LARGE_INTEGER*)&qpc);
    retval = retval * 2;
    if (sec) *sec = (long)(qpc / freq) + addsec;
    if (usec) *usec = (long)((qpc % freq) * 1000000 / freq);
#endif
}

/* get clock in millisecond 64 */
IINT64 CKcpServer::iclock64(void)
{
    long s, u;
    IINT64 value;
    itimeofday(&s, &u);
    value = ((IINT64)s) * 1000 + (u / 1000);
    return value;
}

IUINT32 CKcpServer::iclock()
{
    return (IUINT32)(iclock64() & 0xfffffffful);
}

static int kcp_udpOutPut(const char* buf, int len, ikcpcb* kcp, void* user)
{
    //发送kcp数据
    CKcpServer* kcp_server = (CKcpServer*)user;
    kcp_server->send_io_data_to_point(buf, len, kcp_server->get_kcp_send_endpoint());
    return 0;
}
