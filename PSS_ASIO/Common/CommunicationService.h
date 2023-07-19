#pragma once

#include "ICommunicationService.h"
#include "TcpClientSession.h"
#include "UdpClientSession.h"
#include "TcpSSLClientSession.h"
#include "TtyServer.h"
#include "tms.hpp"
#include "CoreTimer.hpp"

//管理服务器间链接，自动重连的类
//add by freeeyes

class CCommunicationIOInfo
{
public:
    shared_ptr<ISession> session_;
    uint32 connect_id_;
    CConnect_IO_Info io_info_;
    EM_CONNECT_IO_TYPE io_type_;
};

using Communication_funtion = std::function<void(CCommunicationIOInfo&)>;

class CCommunicationService : public ICommunicationInterface
{
public:
    ~CCommunicationService() final = default;

    void init_communication_service(CreateIoContextCallbackFunc callback, uint16 timeout_seconds) final;

    bool add_connect(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type) final;

    void set_connect_id(uint32 server_id, uint32 connect_id) final;

    uint32 get_connect_id(uint32 server_id) final;

    void close_connect(uint32 server_id) final;

    bool is_exist(uint32 server_id) final;

    void close() final;

    uint32 get_server_id(uint32 connect_id) final;

    void reset_connect(uint32 server_id) final;

    void run_check_task();

    void io_connect(CCommunicationIOInfo& connect_info);

    void run_server_to_server();

    void each(Communication_funtion communication_funtion);

private:
    using communication_list = unordered_map<uint32, CCommunicationIOInfo>;
    using server_connect_id_list = unordered_map<uint32, uint32>;
    communication_list communication_list_;
    server_connect_id_list server_connect_id_list_;
    std::recursive_mutex mutex_;
    CreateIoContextCallbackFunc callback_;
    bool communication_is_run_ = false;
};

using App_CommunicationService = PSS_singleton<CCommunicationService>;

