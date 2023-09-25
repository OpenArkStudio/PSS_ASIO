#include "NetSvrManager.h"


CNetSvrManager::CNetSvrManager()
{
}

CNetSvrManager::~CNetSvrManager()
{
}

void CNetSvrManager::start_default_service()
{
    string stripport;

    //加载Tcp监听
    for(auto& tcp_server : App_ServerConfig::instance()->get_config_tcp_list())
    {
        if (tcp_server.ssl_server_password_ != ""
            && tcp_server.ssl_server_pem_file_ != ""
            && tcp_server.ssl_dh_pem_file_ != "")
        {
#ifdef SSL_SUPPORT
            auto tcp_ssl_service = make_shared<CTcpSSLServer>(CreateIoContextFunctor,
                tcp_server.ip_,
                tcp_server.port_,
                tcp_server.packet_parse_id_,
                tcp_server.recv_buff_size_,
                tcp_server.ssl_server_password_,
                tcp_server.ssl_server_pem_file_,
                tcp_server.ssl_dh_pem_file_,
                this);
            tcp_ssl_service->start();
#else
            PSS_LOGGER_DEBUG("[CNetSvrManager::start_default_service]you must set SSL_SUPPORT macro on compilation options.");
#endif
        }
        else
        {
            //正常的tcp链接
            auto tcp_service = make_shared<CTcpServer>(CreateIoContextFunctor,
                tcp_server.ip_,
                tcp_server.port_,
                tcp_server.packet_parse_id_,
                tcp_server.recv_buff_size_,
                this);
            tcp_service->start();
        }
    }

    //加载UDP监听
    for (auto& udp_server : App_ServerConfig::instance()->get_config_udp_list())
    {
        auto udp_service = make_shared<CUdpServer>(CreateIoContextFunctor(), 
            udp_server.ip_,
            udp_server.port_,
            udp_server.packet_parse_id_,
            udp_server.recv_buff_size_,
            udp_server.send_buff_size_,
            udp_server.em_net_type_,
            this);
        udp_service->start();
    }

    //加载KCP监听
    for (auto& kcp_server : App_ServerConfig::instance()->get_config_kcp_list())
    {
        auto kcp_service = make_shared<CKcpServer>(CreateIoContextFunctor(),
            kcp_server.ip_,
            kcp_server.port_,
            kcp_server.packet_parse_id_,
            kcp_server.recv_buff_size_,
            kcp_server.send_buff_size_, 
            this);
        kcp_service->start();
    }

    //加载tty监听
    for (auto& tty_server : App_ServerConfig::instance()->get_config_tty_list())
    {
        auto tty_service = make_shared<CTTyServer>(
            tty_server.packet_parse_id_,
            tty_server.recv_buff_size_,
            tty_server.send_buff_size_,
            this);
        tty_service->start(CreateIoContextFunctor(), 
            tty_server.tty_name_, 
            (uint16)tty_server.tty_port_,
            (uint8)tty_server.char_size_,
            0);
    }
}

void CNetSvrManager::close_accept_list(std::vector<shared_ptr<CIo_Net_server>>& io_listen_list)
{
    for (const auto& io_listen : io_listen_list)
    {
        io_listen->close();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    io_listen_list.clear();
}

void CNetSvrManager::close_all_service()
{
    PSS_LOGGER_DEBUG("[CNetSvrManager::close_all_service]begin.");

    std::vector<shared_ptr<CIo_Net_server>> tcp_listen_list;
    std::vector<shared_ptr<CIo_Net_server>> tcp_listen_ssl_list;
    std::vector<shared_ptr<CIo_Net_server>> udp_listen_list;
    std::vector<shared_ptr<CIo_Net_server>> kcp_listen_list;
    std::vector<shared_ptr<CIo_Net_server>> tty_listen_list;

    //停止所有的TCP监听(TCP)
    for (const auto& tcp_service : tcp_service_list_)
    {
        tcp_listen_list.emplace_back(tcp_service.second);
    }
    close_accept_list(tcp_listen_list);

#ifdef SSL_SUPPORT
    //停止所有的SSL监听
    for (const auto& tcp_ssl_service : tcp_ssl_service_map_)
    {
        tcp_listen_ssl_list.emplace_back(tcp_ssl_service.second);
    }
    close_accept_list(tcp_listen_ssl_list);
#endif

    //清理所有kcp资源
    for (const auto& kcp_service : kcp_service_list_)
    {
        kcp_listen_list.emplace_back(kcp_service.second);
    }
    close_accept_list(kcp_listen_list);

    //清理所有udp资源
    for (const auto& udp_service : udp_service_list_)
    {
        udp_listen_list.emplace_back(udp_service.second);
    }
    close_accept_list(udp_listen_list);

    //清理所有tty资源
    for (const auto& tty_service : tty_service_list_)
    {
        tty_listen_list.emplace_back(tty_service.second);
    }
    close_accept_list(tty_listen_list);

    tcp_service_list_.clear();

    kcp_service_list_.clear();

    udp_service_list_.clear();

#ifdef SSL_SUPPORT
    tcp_ssl_service_map_.clear();
#endif

    PSS_LOGGER_DEBUG("[CNetSvrManager::close_all_service]end.");
}

void CNetSvrManager::add_accept_net_io_event(string io_ip, io_port_type io_port, EM_CONNECT_IO_TYPE em_io_net_type, shared_ptr<CIo_Net_server> Io_Net_server)
{
    std::lock_guard <std::mutex> lock(list_mutex_);
    auto io_key = io_ip + "_" + std::to_string(io_port);
    if (EM_CONNECT_IO_TYPE::CONNECT_IO_TCP == em_io_net_type)
    {
        tcp_service_list_[io_key] = Io_Net_server;
    }
    else if (EM_CONNECT_IO_TYPE::CONNECT_IO_UDP == em_io_net_type)
    {
        udp_service_list_[io_key] = Io_Net_server;
    }
    else if (EM_CONNECT_IO_TYPE::CONNECT_IO_TTY == em_io_net_type)
    {
        tty_service_list_[io_key] = Io_Net_server;
    }
}

void CNetSvrManager::del_accept_net_io_event(string io_ip, io_port_type io_port, EM_CONNECT_IO_TYPE em_io_net_type)
{
    std::lock_guard <std::mutex> lock(list_mutex_);
    auto io_key = io_ip + "_" + std::to_string(io_port);
    if (EM_CONNECT_IO_TYPE::CONNECT_IO_TCP == em_io_net_type)
    { 
        tcp_service_list_.erase(io_key);
    }
    else if (EM_CONNECT_IO_TYPE::CONNECT_IO_UDP == em_io_net_type)
    {
        udp_service_list_.erase(io_key);
    }
    else if (EM_CONNECT_IO_TYPE::CONNECT_IO_TTY == em_io_net_type)
    {
        tty_service_list_.erase(io_key);
    }
}

void CNetSvrManager::start_single_service(const CConfigNetIO& netio)
{
    //查找当前的监听是否已经存在
    bool is_find = false;
    list_mutex_.lock();
    string io_key = netio.ip_ + "_" + std::to_string(netio.port_);
    if (EM_CONNECT_IO_TYPE::CONNECT_IO_TCP == netio.protocol_type_)
    {
        auto tcp = tcp_service_list_.find(io_key);
        if (tcp != tcp_service_list_.end())
        {
            //找到了，直接不能创建
            PSS_LOGGER_INFO("[CNetSvrManager::start_single_service]tcp service[{}:{}] already exists", netio.ip_, netio.port_);
            is_find = true;
        }
    }
    else if (EM_CONNECT_IO_TYPE::CONNECT_IO_UDP == netio.protocol_type_)
    {
        auto udp = udp_service_list_.find(io_key);
        if (udp != udp_service_list_.end())
        {
            //找到了，直接不能创建
            PSS_LOGGER_INFO("[CNetSvrManager::start_single_service]udp service[{}:{}] already exists", netio.ip_, netio.port_);
            is_find = true;
        }
    }

    list_mutex_.unlock();

    //如果找到了，不在创建
    if (true == is_find)
    {
        return;
    }

    if(EM_CONNECT_IO_TYPE::CONNECT_IO_TCP == netio.protocol_type_)
    {
        App_tms::instance()->AddMessage(0, [this, netio]() {
            PSS_LOGGER_INFO("[CNetSvrManager::start_single_service]create tcp service[{}:{}]", netio.ip_, netio.port_);
            auto tcp_service = make_shared<CTcpServer>(CreateIoContextFunctor,
                netio.ip_,
                netio.port_,
                netio.packet_parse_id_,
                netio.recv_buff_size_,
                this);
            tcp_service->start();
            });
    }
    else if(EM_CONNECT_IO_TYPE::CONNECT_IO_UDP == netio.protocol_type_)
    {
        App_tms::instance()->AddMessage(0, [this, netio]() {
            PSS_LOGGER_INFO("[CNetSvrManager::start_single_service]create udp service[{}:{}]", netio.ip_, netio.port_);
            auto udp_service = make_shared<CUdpServer>(CreateIoContextFunctor(),
                netio.ip_,
                netio.port_,
                netio.packet_parse_id_,
                netio.recv_buff_size_,
                netio.send_buff_size_,
                netio.em_net_type_, 
                this);
            udp_service->start();
            });
    }
    else
    {
        PSS_LOGGER_INFO("[CNetSvrManager::start_single_service]create protocol_type_ fail, unsupport.[{}:{}]", netio.ip_, netio.port_);
    }
}

void CNetSvrManager::close_single_service(const CConfigNetIO& netio)
{
    if(EM_CONNECT_IO_TYPE::CONNECT_IO_TCP == netio.protocol_type_)
    {
        App_tms::instance()->AddMessage(0, [this, netio]() {
            string io_key = netio.ip_ + "_" + std::to_string(netio.port_);
            PSS_LOGGER_INFO("[CNetSvrManager::close_single_service]close tcp service[{}:{}]", netio.ip_, netio.port_);
            auto tcp = tcp_service_list_.find(io_key);
            if (tcp != tcp_service_list_.end())
            {
                tcp->second->close();
            }
            });
    }
    else if(EM_CONNECT_IO_TYPE::CONNECT_IO_UDP == netio.protocol_type_)
    {
        App_tms::instance()->AddMessage(0, [this, netio]() {
            string io_key = netio.ip_ + "_" + std::to_string(netio.port_);
            PSS_LOGGER_INFO("[CNetSvrManager::close_single_service]close udp service[{}:{}]", netio.ip_, netio.port_);
            auto udp = udp_service_list_.find(io_key);
            if (udp != udp_service_list_.end())
            {
                udp->second->close();
            }
            });
    }
}

