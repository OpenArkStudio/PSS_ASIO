#include "CommunicationService.h"

void CCommunicationService::init_communication_service(CreateIoContextCallbackFunc callback, uint16 timeout_seconds)
{
    //读取配置文件，链接服务器
    callback_ = callback;

    //定时检查任务，检查服务器间链接的状态。
    App_TimerManager::instance()->GetTimerPtr()->addTimer_loop(chrono::seconds(0), chrono::seconds(timeout_seconds), [this]()
        {
            //添加到数据队列处理
            App_tms::instance()->AddMessage(0, [this]() {
                run_check_task();
                });
        });
}

bool CCommunicationService::add_connect(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type)
{
    std::lock_guard <std::recursive_mutex> lock(mutex_);
    CCommunicationIOInfo connect_info;
    connect_info.connect_id_ = 0;
    connect_info.io_info_ = io_info;
    connect_info.io_type_ = io_type;

    io_connect(connect_info);
    return true;
}

void CCommunicationService::set_connect_id(uint32 server_id, uint32 connect_id)
{
    auto f = communication_list_.find(server_id);
    if (f != communication_list_.end())
    {
        f->second.connect_id_ = connect_id;

        if (connect_id == 0)
        {
            f->second.session_ = nullptr;

            //删除映射关系
            server_connect_id_list_.erase(connect_id);
        }
        else
        {
            //添加映射关系
            server_connect_id_list_[connect_id] = server_id;
        }
    }
}

uint32 CCommunicationService::get_connect_id(uint32 server_id)
{
    auto f = communication_list_.find(server_id);
    if (f != communication_list_.end())
    {
        return  f->second.connect_id_ ;
    }

    return 0;
}

void CCommunicationService::io_connect(CCommunicationIOInfo& connect_info)
{
    communication_list_[connect_info.io_info_.server_id] = connect_info;
    
    if (false == communication_is_run_)
    {
        //还在初始化中，不启动链接
        PSS_LOGGER_DEBUG("[CCommunicationService::io_connect]CCommunicationService is not run");
        return;
    }

    if (connect_info.io_type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_TCP)
    {
        //IO是TCP
        auto tcp_client_session = make_shared<CTcpClientSession>(callback_());
        if (true == tcp_client_session->start(connect_info.io_info_))
        {
            communication_list_[connect_info.io_info_.server_id].session_ = tcp_client_session;
        }
    }
    else if(connect_info.io_type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_UDP)
    {
        //IO是UDP
        auto udp_client_session = make_shared<CUdpClientSession>(callback_());
        udp_client_session->start(connect_info.io_info_);
        communication_list_[connect_info.io_info_.server_id].session_ = udp_client_session;
    }
    else if (connect_info.io_type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_TTY)
    {
        //IO是TTY
        auto tty_client_session = make_shared<CTTyServer>(
            connect_info.io_info_.packet_parse_id,
            connect_info.io_info_.recv_size,
            connect_info.io_info_.send_size,
            App_NetSvrManager::instance());
        tty_client_session->start(callback_(),
            connect_info.io_info_.server_ip,
            connect_info.io_info_.server_port,
            8,
            connect_info.io_info_.server_id);
        communication_list_[connect_info.io_info_.server_id].session_ = tty_client_session;
    }
    else if (connect_info.io_type_ == EM_CONNECT_IO_TYPE::CONNECT_IO_SSL)
    {
#ifdef SSL_SUPPORT
        auto ssl_client_session = make_shared<CTcpSSLClientSession>(callback_());
        ssl_client_session->start(connect_info.io_info_);
        communication_list_[connect_info.io_info_.server_id].session_ = ssl_client_session;
#else
        PSS_LOGGER_DEBUG("[CCommunicationService::io_connect]you mest use SSL_SUPPORT macro support ths ssl.");
#endif
    }
}

void CCommunicationService::run_server_to_server()
{
    //开始运行
    communication_is_run_ = true;

    run_first_io_connect();
}

void CCommunicationService::close_connect(uint32 server_id)
{
    std::lock_guard <std::recursive_mutex> lock(mutex_);
    communication_list_.erase(server_id);

    auto connect_id = get_server_id(server_id);
    if (connect_id > 0)
    {
        server_connect_id_list_.erase(connect_id);
    }
}

bool CCommunicationService::is_exist(uint32 server_id)
{
    std::lock_guard <std::recursive_mutex> lock(mutex_);
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

void CCommunicationService::each_io_connect(Communication_funtion communication_funtion)
{
    for (auto& client_info : communication_list_)
    {
        communication_funtion(client_info.second);
    }
}

void CCommunicationService::run_check_task()
{
    std::lock_guard <std::recursive_mutex> lock(mutex_);
    PSS_LOGGER_DEBUG("[CCommunicationService::run_check_task]begin size={}.", communication_list_.size());

    each_io_connect([this](CCommunicationIOInfo& io_info) {
        if ((io_info.session_ == nullptr || false == io_info.session_->is_connect()) && true == io_info.io_info_.is_need_reconnect)
        {
            //重新建立链接
            io_connect(io_info);
        }
        });

    PSS_LOGGER_DEBUG("[CCommunicationService::run_check_task]end.");
}

void CCommunicationService::run_first_io_connect()
{
    std::lock_guard <std::recursive_mutex> lock(mutex_);

    //第一次建立所有的连接
    each_io_connect([this](CCommunicationIOInfo& io_info) {
        //重新建立链接
        io_connect(io_info);
        });
}

void CCommunicationService::close()
{
    std::lock_guard <std::recursive_mutex> lock(mutex_);

    server_connect_id_list_.clear();
    communication_list_.clear();
}

uint32 CCommunicationService::get_server_id(uint32 connect_id)
{
    std::lock_guard <std::recursive_mutex> lock(mutex_);
    auto f = server_connect_id_list_.find(connect_id);
    if (f != server_connect_id_list_.end())
    {
        return f->second;
    }
    else
    {
        return 0;
    }
}

void CCommunicationService::reset_connect(uint32 server_id)
{
    std::lock_guard <std::recursive_mutex> lock(mutex_);
    auto f = communication_list_.find(server_id);
    if (f != communication_list_.end())
    {
        io_connect(f->second);
    }
}
