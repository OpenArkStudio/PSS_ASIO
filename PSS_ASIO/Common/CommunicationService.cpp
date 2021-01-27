#include "CommunicationService.h"

void CCommunicationService::init_communication_service(asio::io_context* io_service_context, uint16 timeout_seconds)
{
    //读取配置文件，链接服务器
    io_service_context_ = io_service_context;

    //测试定时器
    App_TimerManager::instance()->GetTimerPtr()->addTimer_loop(chrono::seconds(timeout_seconds), [this]()
        {
            run_check_task();
        });
}

bool CCommunicationService::add_connect(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type)
{
    std::lock_guard <std::mutex> lock(mutex_);
    CCommunicationIOInfo connect_info;
    connect_info.connect_id_ = 0;
    connect_info.io_info_ = io_info;
    connect_info.io_type_ = io_type;

    io_connect(connect_info);
    return true;
}

void CCommunicationService::set_connect_id(uint32 server_id, uint32 connect_id)
{
    std::lock_guard <std::mutex> lock(mutex_);
    auto f = communication_list_.find(server_id);
    if (f != communication_list_.end())
    {
        f->second.connect_id_ = connect_id;

        if (connect_id == 0)
        {
            f->second.session_ = nullptr;
        }
    }
}

void CCommunicationService::io_connect(CCommunicationIOInfo& connect_info)
{
    if (connect_info.io_type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_TCP)
    {
        //默认是TCP
        auto tcp_client_session = make_shared<CTcpClientSession>(io_service_context_);
        if (true == tcp_client_session->start(connect_info.io_info_))
        {
            connect_info.session_ = tcp_client_session;
        }
    }
    else
    {
        //默认都是UDP
        auto udp_client_session = make_shared<CUdpClientSession>(io_service_context_);
        udp_client_session->start(connect_info.io_info_);
        connect_info.session_ = udp_client_session;
    }

    communication_list_[connect_info.io_info_.server_id] = connect_info;
}

void CCommunicationService::close_connect(uint32 server_id)
{
    std::lock_guard <std::mutex> lock(mutex_);
    communication_list_.erase(server_id);
}

bool CCommunicationService::is_exist(uint32 server_id)
{
    std::lock_guard <std::mutex> lock(mutex_);
    auto f = communication_list_.find(server_id);
    if (f != communication_list_.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

void CCommunicationService::run_check_task()
{
    std::lock_guard <std::mutex> lock(mutex_);
    PSS_LOGGER_DEBUG("[CCommunicationService::run_check_task]begin.");

    for (auto& client_info : communication_list_)
    {
        if (client_info.second.session_ == nullptr)
        {
            //重新建立链接
            io_connect(client_info.second);
        }
    }

    PSS_LOGGER_DEBUG("[CCommunicationService::run_check_task]end.");
}

void CCommunicationService::close()
{
    std::lock_guard <std::mutex> lock(mutex_);
    communication_list_.clear();
}
