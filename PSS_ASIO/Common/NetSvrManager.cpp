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
    for(auto tcp_server : App_ServerConfig::instance()->get_config_tcp_list())
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
                tcp_server.ssl_dh_pem_file_);
            stripport = tcp_server.ip_ + "_" + std::to_string(tcp_server.port_);
            tcp_ssl_service_map_[stripport] = tcp_ssl_service;
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
                tcp_server.recv_buff_size_);
            stripport = tcp_server.ip_ + "_" + std::to_string(tcp_server.port_);
            tcp_service_map_[stripport] = tcp_service;
        }
    }

    //加载UDP监听
    for (auto udp_server : App_ServerConfig::instance()->get_config_udp_list())
    {
        auto udp_service = make_shared<CUdpServer>(CreateIoContextFunctor(), 
            udp_server.ip_,
            udp_server.port_,
            udp_server.packet_parse_id_,
            udp_server.recv_buff_size_,
            udp_server.send_buff_size_,
            udp_server.em_net_type_);
        udp_service->start();
        stripport = udp_server.ip_ + "_" + std::to_string(udp_server.port_);
        udp_service_map_[stripport] = udp_service;
    }

    //加载KCP监听
    for (auto kcp_server : App_ServerConfig::instance()->get_config_kcp_list())
    {
        auto kcp_service = make_shared<CKcpServer>(CreateIoContextFunctor(),
            kcp_server.ip_,
            kcp_server.port_,
            kcp_server.packet_parse_id_,
            kcp_server.recv_buff_size_,
            kcp_server.send_buff_size_);
        kcp_service->start();
        stripport = kcp_server.ip_ + "_" + std::to_string(kcp_server.port_);
        kcp_service_map_[stripport] = kcp_service;
    }

    //加载tty监听
    for (auto tty_server : App_ServerConfig::instance()->get_config_tty_list())
    {
        auto tty_service = make_shared<CTTyServer>(
            tty_server.packet_parse_id_,
            tty_server.recv_buff_size_,
            tty_server.send_buff_size_);
        tty_service->start(CreateIoContextFunctor(), 
            tty_server.tty_name_, 
            (uint16)tty_server.tty_port_,
            (uint8)tty_server.char_size_,
            0);
        tty_service_map_[tty_server.tty_name_] = tty_service;
    }
}

void CNetSvrManager::close_all_service()
{
    PSS_LOGGER_DEBUG("[CNetSvrManager::close_all_service]begin.");

    //停止所有的TCP监听(TCP)
    for (const auto& tcp_service : tcp_service_map_)
    {
        tcp_service.second->close();
    }

#ifdef SSL_SUPPORT
    //停止所有的SSL监听
    for (const auto& tcp_ssl_service : tcp_ssl_service_map_)
    {
        tcp_ssl_service.second->close();
    }
#endif

    //清理所有kcp资源
    for (const auto& kcp_service : kcp_service_map_)
    {
        kcp_service.second->close_all();
    }

    //清理所有udp资源
    for (const auto& udp_service : udp_service_map_)
    {
        udp_service.second->close_all();
    }

    tcp_service_map_.clear();

    kcp_service_map_.clear();

    udp_service_map_.clear();

#ifdef SSL_SUPPORT
    tcp_ssl_service_map_.clear();
#endif
    PSS_LOGGER_DEBUG("[CNetSvrManager::close_all_service]end.");
}

void CNetSvrManager::start_single_service(const CConfigNetIO& netio)
{
    if("TCP" == netio.protocol_type_)
    {
        App_tms::instance()->AddMessage(0, [this, netio]() {
            string stripport;
            PSS_LOGGER_INFO("[CNetSvrManager::start_single_service]create tcp service[{}:{}]", netio.ip_, netio.port_);
            auto tcp_service = make_shared<CTcpServer>(CreateIoContextFunctor,
                netio.ip_,
                netio.port_,
                netio.packet_parse_id_,
                netio.recv_buff_size_);
            stripport = netio.ip_ + "_" + std::to_string(netio.port_);
            tcp_service_map_[stripport] = tcp_service;
            });
    }
    else
    {
        App_tms::instance()->AddMessage(0, [this, netio]() {
            string stripport;
            PSS_LOGGER_INFO("[CNetSvrManager::start_single_service]create udp service[{}:{}]", netio.ip_, netio.port_);
            auto udp_service = make_shared<CUdpServer>(CreateIoContextFunctor(),
                netio.ip_,
                netio.port_,
                netio.packet_parse_id_,
                netio.recv_buff_size_,
                netio.send_buff_size_,
                netio.em_net_type_);
            udp_service->start();
            stripport = netio.ip_ + "_" + std::to_string(netio.port_);
            udp_service_map_[stripport] = udp_service;
            });
    }
}

void CNetSvrManager::close_single_service(const CConfigNetIO& netio)
{
    if("TCP" == netio.protocol_type_)
    {
        App_tms::instance()->AddMessage(0, [this, netio]() {
            string stripport = netio.ip_ + "_" + std::to_string(netio.port_);
            PSS_LOGGER_INFO("[CNetSvrManager::close_single_service]close tcp service[{}:{}]", netio.ip_, netio.port_);
            auto tcp = tcp_service_map_.find(stripport);
            if (tcp != tcp_service_map_.end())
            {
                tcp->second->close();
                tcp_service_map_.erase(stripport);
            }
            });
    }
    else
    {
        App_tms::instance()->AddMessage(0, [this, netio]() {
            string stripport = netio.ip_ + "_" + std::to_string(netio.port_);
            PSS_LOGGER_INFO("[CNetSvrManager::close_single_service]close udp service[{}:{}]", netio.ip_, netio.port_);
            auto udp = udp_service_map_.find(stripport);
            if (udp != udp_service_map_.end())
            {
                udp->second->close_all();
                udp_service_map_.erase(stripport);
            }
            });
    }
}

