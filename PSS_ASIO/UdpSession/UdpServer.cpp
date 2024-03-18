#include "UdpServer.h"

CUdpServer::CUdpServer(asio::io_context* io_context, const CConfigNetIO& config_io, CIo_List_Manager* io_list_manager)
    :socket_(*io_context), max_recv_size_(config_io.recv_buff_size_), max_send_size_(config_io.send_buff_size_), io_context_(io_context), server_ip_(config_io.ip_), server_port_(config_io.port_), io_list_manager_(io_list_manager)
{
    //处理链接建立消息
    PSS_LOGGER_DEBUG("[CUdpServer::CUdpServer]{0}:{1} Begin Accept.", server_ip_, server_port_);

    try
    {
        socket_.open(udp::v4());
        socket_.set_option(asio::ip::udp::socket::reuse_address(true));
        // 设置UDP缓冲区大小为udp_net_buffer_size
        asio::socket_base::receive_buffer_size recvoption(udp_net_buffer_size);
        asio::socket_base::send_buffer_size sendoption(udp_net_buffer_size);
        socket_.set_option(recvoption);
        socket_.set_option(sendoption);
    
        udp::endpoint local_endpoint(asio::ip::address_v4::from_string(server_ip_), server_port_);
        socket_.bind(local_endpoint);    // 将套接字绑定到本地地址和端口
    }
    catch (std::system_error const& ex)
    {
        PSS_LOGGER_ERROR("[CUdpServer::CUdpServer]bind addr error local:[{}:{}] ex.what:{}.", server_ip_, server_port_, ex.what());
    }

    if (config_io.em_net_type_ == EM_NET_TYPE::NET_TYPE_BROADCAST)
    {
        //处理UDP的广播监听
        asio::error_code ec;
        asio::socket_base::broadcast option(true);
        socket_.set_option(option, ec);
        if (ec)
        {
            PSS_LOGGER_DEBUG("[CUdpServer::CUdpServer]{0}:{1} error bind Accept{2}.", server_ip_, server_port_, ec.message());
        }
    }

    session_recv_buffer_.Init(max_recv_size_);

    packet_parse_interface_ = App_PacketParseLoader::instance()->GetPacketParseInfo(config_io.packet_parse_id_);
}

void CUdpServer::start()
{
    io_list_manager_->add_accept_net_io_event(server_ip_, server_port_, EM_CONNECT_IO_TYPE::CONNECT_IO_UDP, shared_from_this());
    do_receive();
}

_ClientIPInfo CUdpServer::get_remote_ip(uint32 connect_id)
{
    std::lock_guard <std::recursive_mutex> lock(udp_session_mutex_);
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

void CUdpServer::do_receive()
{
    auto self(shared_from_this());
    socket_.async_receive_from(
        asio::buffer(session_recv_buffer_.get_curr_write_ptr(), session_recv_buffer_.get_buffer_size()), recv_endpoint_,
        [self](std::error_code ec, std::size_t length)
        {
            try
            {
                if (!ec)
                {
                    self->do_receive_from(ec, length);
                }
                else
                {
                    PSS_LOGGER_DEBUG("[CUdpServer::do_receive]({}:{})async_receive_from:{}.",
                        self->server_ip_,
                        self->server_port_,
                        ec.message());

                    App_WorkThreadLogic::instance()->add_frame_events(LOGIC_LISTEN_SERVER_ERROR,
                        0,
                        self->server_ip_,
                        self->server_port_,
                        EM_CONNECT_IO_TYPE::CONNECT_IO_UDP);

                    self->io_list_manager_->del_accept_net_io_event(self->server_ip_, self->server_port_, EM_CONNECT_IO_TYPE::CONNECT_IO_UDP);
                }
            } 
            catch (std::system_error const& ex) 
            {
                PSS_LOGGER_WARN("[CUdpServer::do_receive]close udp server[{}:{}], error={}",self->server_ip_,self->server_port_, ex.what());
            }
        });
}

void CUdpServer::do_receive_from(std::error_code ec, std::size_t length)
{
    try
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
                App_WorkThreadLogic::instance()->close_session_event(connect_id, shared_from_this());
                do_receive();
                return;
            }

            session_recv_buffer_.set_write_data(length);

            auto session_info = find_udp_endpoint_by_id(connect_id);

            if (nullptr != session_info && EM_SESSION_STATE::SESSION_IO_BRIDGE == session_info->io_state_)
            {
                recv_data_time_ = std::chrono::steady_clock::now();
                cid_recv_data_time_[connect_id] = std::chrono::steady_clock::now();
                //将数据转发给桥接接口
                auto ret = App_WorkThreadLogic::instance()->do_io_bridge_data(session_info->connect_id_, session_info->io_bridge_connect_id_, session_recv_buffer_, length, shared_from_this());
                if (1 == ret)
                {
                    //远程IO链接已断开
                    session_info->io_bridge_connect_id_ = 0;
                    session_info->io_state_ = EM_SESSION_STATE::SESSION_IO_LOGIC;
                }
            }
            else
            {
                //处理数据拆包
                vector<std::shared_ptr<CMessage_Packet>> message_list;
                bool ret = packet_parse_interface_->packet_from_recv_buffer_ptr_(connect_client_id_, &session_recv_buffer_, message_list, io_type_);
                if (!ret)
                {
                    //链接断开(解析包不正确)
                    session_recv_buffer_.move(length);
                    App_WorkThreadLogic::instance()->close_session_event(connect_id, shared_from_this());
                }
                else
                {
                    recv_data_time_ = std::chrono::steady_clock::now();
                    cid_recv_data_time_[connect_id] = std::chrono::steady_clock::now();

                    //添加到数据队列处理
                    App_WorkThreadLogic::instance()->assignation_thread_module_logic(connect_id, message_list, self);
                }
            }

            session_recv_buffer_.move(length);

#ifdef GCOV_TEST
            //测试代码
            auto remote_ip_info = get_remote_ip(connect_id);
            PSS_LOGGER_WARN("[CUdpServer::do_receive_from]client udp {}:{} is connect server",
                remote_ip_info.m_strClientIP,
                remote_ip_info.m_u2Port);

            set_io_bridge_connect_id(connect_id, 3);
            set_io_bridge_connect_id(connect_id, 0);
#endif

            //持续接收数据
            do_receive();
        }
    } 
    catch (std::system_error const& ex) 
    {
        PSS_LOGGER_WARN("[CUdpServer::do_receive_from]close udp server[{}:{}], error={}",server_ip_,server_port_, ex.what());
    }
}

void CUdpServer::close(uint32 connect_id)
{
    PSS_LOGGER_DEBUG("[CUdpServer::close]start connect_id={0}",connect_id);
    close_udp_endpoint_by_id(connect_id);
    PSS_LOGGER_DEBUG("[CUdpServer::close]end connect_id={0}",connect_id);
}

void CUdpServer::close()
{
    close_server();
}

void CUdpServer::close_server()
{
    std::lock_guard <std::recursive_mutex> lock(udp_session_mutex_);
    if (!socket_.is_open())
    {
        return;
    }
    //释放所有udp资源
    for (const auto& session_info : udp_id_2_endpoint_list_)
    {
        auto connect_id = session_info.first;
        //这里发送数据通知()
        packet_parse_interface_->packet_disconnect_ptr_(connect_id, io_type_, App_IoBridge::instance());

        //发送处理IO断开事件
        _ClientIPInfo remote_ip_info;
        remote_ip_info.m_strClientIP = session_info.second->send_endpoint.address().to_string();
        remote_ip_info.m_u2Port = session_info.second->send_endpoint.port();

        App_WorkThreadLogic::instance()->delete_thread_session(connect_id, 
            shared_from_this(), 
            remote_ip_info,
            io_type_);
    }

    udp_id_2_endpoint_list_.clear();
    udp_endpoint_2_id_list_.clear();
    socket_.close();

    PSS_LOGGER_DEBUG("[CUdpServer::close_server]close [{0}:{1}]", server_ip_, server_port_);
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

    io_context_->dispatch([self, connect_id, send_buffer, session_info]()
        {
            self->socket_.async_send_to(
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
        });
}

void CUdpServer::do_write_immediately(uint32 connect_id, const char* data, size_t length)
{
    auto session_info = find_udp_endpoint_by_id(connect_id);

    if (session_info == nullptr)
    {
        PSS_LOGGER_DEBUG("[CUdpServer::do_write_immediately]connect_id={0} is nullptr.", connect_id);
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
    uint32 connect_id = 0;
    {
        std::lock_guard <std::recursive_mutex> lock(udp_session_mutex_);

        auto f = udp_endpoint_2_id_list_.find(recv_endpoint);
        if (f != udp_endpoint_2_id_list_.end())
        {
            //找到了，返回ID
            return f->second;
        }
        else
        {
            //生成一个新的ID
            connect_id = App_ConnectCounter::instance()->CreateCounter();

            auto session_info = make_shared<CUdp_Session_Info>();
            session_info->send_endpoint = recv_endpoint;
            session_info->recv_data_size_ += length;
            session_info->connect_id_ = connect_id;
            session_info->udp_state = EM_UDP_VALID::UDP_VALUD;
            session_info->session_send_buffer_.Init(max_buffer_length);

            udp_endpoint_2_id_list_[recv_endpoint] = connect_id;
            udp_id_2_endpoint_list_[connect_id] = session_info;
        }
    }

    //调用packet parse 链接建立
    _ClientIPInfo remote_ip;
    _ClientIPInfo local_ip;
    remote_ip.m_strClientIP = recv_endpoint.address().to_string();
    remote_ip.m_u2Port = recv_endpoint.port();
    local_ip.m_strClientIP = socket_.local_endpoint().address().to_string();
    local_ip.m_u2Port = socket_.local_endpoint().port();
    packet_parse_interface_->packet_connect_ptr_(connect_id, remote_ip, local_ip, io_type_, App_IoBridge::instance());

    //添加映射关系
    App_WorkThreadLogic::instance()->add_thread_session(connect_id, shared_from_this(), local_ip, remote_ip);

    return connect_id;
}

shared_ptr<CUdp_Session_Info> CUdpServer::find_udp_endpoint_by_id(uint32 connect_id)
{
    std::lock_guard <std::recursive_mutex> lock(udp_session_mutex_);

    auto f = udp_id_2_endpoint_list_.find(connect_id);
    if (f != udp_id_2_endpoint_list_.end())
    {
        return f->second;
    }
    
    return nullptr;
}

void CUdpServer::close_udp_endpoint_by_id(uint32 connect_id)
{
    std::lock_guard <std::recursive_mutex> lock(udp_session_mutex_);
    auto self(shared_from_this());
        
    if (!socket_.is_open())
    {
        return;
    }

    _ClientIPInfo remote_ip;

    auto f = udp_id_2_endpoint_list_.find(connect_id);
    if (f != udp_id_2_endpoint_list_.end())
    {
        //调用packet parse 断开消息
        packet_parse_interface_->packet_disconnect_ptr_(connect_id, io_type_, App_IoBridge::instance());

        remote_ip.m_strClientIP = f->second->send_endpoint.address().to_string();
        remote_ip.m_u2Port = f->second->send_endpoint.port();

        App_WorkThreadLogic::instance()->delete_thread_session(connect_id, self, remote_ip, io_type_);

        //清理链接关系
        auto session_endpoint = f->second->send_endpoint;
        udp_id_2_endpoint_list_.erase(connect_id);
        udp_endpoint_2_id_list_.erase(session_endpoint);
    }

    auto iter=cid_recv_data_time_.find(connect_id);
    if(iter != cid_recv_data_time_.end())
    {
        cid_recv_data_time_.erase(iter);
    }
}

void CUdpServer::add_send_finish_size(uint32 connect_id, size_t length)
{
    std::lock_guard <std::recursive_mutex> lock(udp_session_mutex_);
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

uint32 CUdpServer::get_connect_id() 
{
    return 0;
}

std::chrono::steady_clock::time_point& CUdpServer::get_recv_time(uint32 connect_id)
{
    auto iter=cid_recv_data_time_.find(connect_id);
    if(iter != cid_recv_data_time_.end())
    {
        return iter->second;
    }
    
    return recv_data_time_;
}

bool CUdpServer::format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message)
{
    return packet_parse_interface_->parse_format_send_buffer_ptr_(connect_id, message, format_message, get_io_type());
}

bool CUdpServer::is_need_send_format()
{
    return packet_parse_interface_->is_need_send_format_ptr_();
}

void CUdpServer::set_io_bridge_connect_id(uint32 from_io_connect_id, uint32 to_io_connect_id)
{
    auto session_info = find_udp_endpoint_by_id(from_io_connect_id);

    if (session_info == nullptr)
    {
        PSS_LOGGER_DEBUG("[CUdpServer::set_io_bridge_connect_id]({}) is not find.", from_io_connect_id);
        return;
    }
    else
    {
        if (to_io_connect_id > 0)
        {
            session_info->io_state_ = EM_SESSION_STATE::SESSION_IO_BRIDGE;
            session_info->io_bridge_connect_id_ = to_io_connect_id;
        }
        else
        {
            session_info->io_state_ = EM_SESSION_STATE::SESSION_IO_LOGIC;
            session_info->io_bridge_connect_id_ = 0;
        }
    }
}

