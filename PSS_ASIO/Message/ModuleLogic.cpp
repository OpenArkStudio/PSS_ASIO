#include "ModuleLogic.h"

void CModuleLogic::init_logic(const command_to_module_function& command_to_module_function, uint16 work_thread_id)
{
    modules_interface_.copy_from_module_list(command_to_module_function);
    work_thread_id_ = work_thread_id;

    sessions_interface_.start_check();
}

void CModuleLogic::add_session(uint32 connect_id, shared_ptr<ISession> session, const _ClientIPInfo& local_info, const _ClientIPInfo& romote_info)
{
    sessions_interface_.add_session_interface(connect_id, session, local_info, romote_info);
}

shared_ptr<ISession> CModuleLogic::get_session_interface(uint32 connect_id)
{
    auto ret = sessions_interface_.get_session_interface(connect_id);

#ifdef GCOV_TEST
    auto local_ip = sessions_interface_.get_session_local_ip(connect_id);
    PSS_LOGGER_DEBUG("[CModuleLogic::get_session_interface]local IP={0}:{1}",
        local_ip.m_strClientIP,
        local_ip.m_u2Port);

    auto remote_ip = sessions_interface_.get_session_remote_ip(connect_id);
    PSS_LOGGER_DEBUG("[CModuleLogic::get_session_interface]remote IP={0}:{1}",
        remote_ip.m_strClientIP,
        remote_ip.m_u2Port);
#endif
    return ret;
}

void CModuleLogic::delete_session_interface(uint32 connect_id)
{
    sessions_interface_.delete_session_interface(connect_id);
}

void CModuleLogic::close()
{
    modules_interface_.close();
}

int CModuleLogic::do_thread_module_logic(const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    last_dispose_command_id_ = recv_packet->command_id_;
    work_thread_state_ = ENUM_WORK_THREAD_STATE::WORK_THREAD_BEGIN;
    auto ret = modules_interface_.do_module_message(source, recv_packet, send_packet);
    work_thread_state_ = ENUM_WORK_THREAD_STATE::WORK_THREAD_END;
    work_thread_run_time_ = std::chrono::steady_clock::now();
    return ret;
}

uint16 CModuleLogic::get_work_thread_id() const 
{
    return work_thread_id_;
}

int CModuleLogic::get_work_thread_timeout() const
{
    if(ENUM_WORK_THREAD_STATE::WORK_THREAD_BEGIN == work_thread_state_)
    {
        auto interval_seconds = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - work_thread_run_time_);
        return (int)interval_seconds.count();
    }
    else
    {
        return 0;
    }
}

void CModuleLogic::check_session_io_timeout(uint32 connect_timeout)
{
    vector<CSessionIO_Cancel> session_list;
    sessions_interface_.check_session_io_timeout(connect_timeout, session_list);
    for (const auto& session_io : session_list)
    {
        PSS_LOGGER_DEBUG("[CModuleLogic::check_session_io_timeout]work_thread_id_={0}, session_id={1} is timeout.", work_thread_id_, session_io.session_id_);
        App_WorkThreadLogic::instance()->close_session_event(session_io.session_id_, session_io.session_);
    }
}

uint16 CModuleLogic::get_last_dispose_command_id() const
{
    //返回最后一个处理的命令ID
    return last_dispose_command_id_;
}

void CModuleLogic::each_session_id(const session_func& session_fn) const
{
    sessions_interface_.each_session_id(session_fn);
}

void CWorkThreadLogic::init_work_thread_logic(int thread_count, uint16 timeout_seconds, uint32 connect_timeout, const config_logic_list& logic_list, ISessionService* session_service)
{
    //初始化线程数
    thread_count_ = (uint16)thread_count;
    connect_timeout_ = connect_timeout;

    App_tms::instance()->Init();

    load_module_.set_session_service(session_service);

    //初始化插件加载
    for (const auto& logic_library : logic_list)
    {
        load_module_.load_plugin_module(logic_library.logic_path_, 
            logic_library.logic_file_name_, 
            logic_library.logic_param_);
    }

#ifdef GCOV_TEST
    PSS_LOGGER_DEBUG("[CWorkThreadLogic::init_work_thread_logic]count = {0}", load_module_.get_module_count());
    vector<std::string> modeinfo_list;
    load_module_.get_all_module_name(modeinfo_list);
    if (modeinfo_list.size() != (uint32)load_module_.get_module_count())
    {
        PSS_LOGGER_DEBUG("[CWorkThreadLogic::init_work_thread_logic]count is fail", load_module_.get_module_count());
    }

    //查询指定的消息
    auto module_info = load_module_.find_module_info("libTest_Logic.so");
    if (nullptr == module_info)
    {
        PSS_LOGGER_DEBUG("[CWorkThreadLogic::init_work_thread_logic]no find libTest_Logic.so");
    }
#endif

    //显示所有的注册消息以及对应的模块
    PSS_LOGGER_DEBUG("[CWorkThreadLogic::init_work_thread_logic]>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    for (const auto& command_info : load_module_.get_module_function_list())
    {
        PSS_LOGGER_DEBUG("[CWorkThreadLogic::init_work_thread_logic]register command id={0}", command_info.first);
    }
    PSS_LOGGER_DEBUG("[CWorkThreadLogic::init_work_thread_logic]>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");

    //执行线程对应创建
    for (int i = 0; i < thread_count; i++)
    {
        auto thread_logic = make_shared<CModuleLogic>();

        thread_logic->init_logic(load_module_.get_module_function_list(), (uint16)i);

        thread_module_list_.emplace_back(thread_logic);

        //初始化线程
        App_tms::instance()->CreateLogic(i);
    }

    module_init_finish_ = true;

    //创建插件使用的线程
    for (auto thread_id : plugin_work_thread_buffer_list_)
    {
        //查找线程是否已经存在
        auto f = plugin_work_thread_list_.find(thread_id);
        if (f != plugin_work_thread_list_.end())
        {
            continue;
        }

        auto thread_logic = make_shared<CModuleLogic>();

        thread_logic->init_logic(load_module_.get_module_function_list(), (uint16)thread_id);

        plugin_work_thread_list_[thread_id] = thread_logic;

        //初始化线程
        App_tms::instance()->CreateLogic(thread_id);
    }

    //所有线程启动完成
    App_tms::instance()->Start();

    plugin_work_thread_buffer_list_.clear();

    //等待10毫秒，让所有线程创建完毕
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    //加载插件投递事件
    for (const auto& plugin_events : plugin_work_thread_buffer_message_list_)
    {
        do_frame_message(plugin_events.tag_thread_id_,
            plugin_events.message_tag_,
            plugin_events.send_packet_,
            plugin_events.delay_timer_);

    }
    plugin_work_thread_buffer_message_list_.clear();

    //加载插件逻辑
    for (const auto& plugin_logic : plugin_work_thread_buffer_Func_list_)
    {
        do_work_thread_logic(plugin_logic->tag_thread_id_,
            plugin_logic->delay_timer_,
            plugin_logic->func_);
    }
    plugin_work_thread_buffer_Func_list_.clear();

    //定时检查任务，定时检查服务器状态
    App_TimerManager::instance()->GetTimerPtr()->addTimer_loop(chrono::seconds(0), chrono::seconds(timeout_seconds), [this, timeout_seconds]()
        {
            //添加到数据队列处理
            App_tms::instance()->AddMessage(0, [this, timeout_seconds]() {
                run_check_task(timeout_seconds);
                });
        });
    
#ifdef GCOV_TEST
    //测试线程死锁消息
    do_work_thread_timeout(1, 1001, 30);
#endif
}

void CWorkThreadLogic::init_communication_service(ICommunicationInterface* communicate_service)
{
    communicate_service_ = communicate_service;
}

void CWorkThreadLogic::close()
{
    if (nullptr != communicate_service_)
    {
        communicate_service_->close();
    }

    //关闭所有的客户端(异步)
    for (auto f : thread_module_list_)
    {
        App_tms::instance()->AddMessage(f->get_work_thread_id(), [this, f]() {
            f->each_session_id([this, f](uint32 session_id) {
                PSS_LOGGER_DEBUG("[CWorkThreadLogic::close]session_id ({0}) is close", session_id);
                close_session_event(session_id, f->get_session_interface(session_id));
                });
            });
    }

    //关闭所有的扩展工作线程
    for (const auto& f : plugin_work_thread_list_)
    {
        f.second->close();
    }

    //关闭线程操作
    App_tms::instance()->Close();

    //释放对应模块接口
    for (auto f : thread_module_list_)
    {
        f->close();
    }

    thread_module_list_.clear();

    //关闭模板操作
    load_module_.Close();
}

void CWorkThreadLogic::do_work_thread_frame_events(uint16 command_id, uint32 mark_id, const std::string& remote_ip, uint16 remote_port, EM_CONNECT_IO_TYPE io_type)
{
    auto module_logic = thread_module_list_[0];

    CMessage_Source source;
    auto recv_packet = std::make_shared<CMessage_Packet>();
    auto send_packet = std::make_shared<CMessage_Packet>();

    recv_packet->command_id_ = command_id;

    if (recv_packet->command_id_ == LOGIC_CONNECT_SERVER_ERROR)
    {
        source.connect_id_ = 0;
        source.work_thread_id_ = module_logic->get_work_thread_id();
        source.connect_mark_id_ = mark_id;
        source.remote_ip_.m_strClientIP = remote_ip;
        source.remote_ip_.m_u2Port = remote_port;
    }
    else if (recv_packet->command_id_ == LOGIC_LISTEN_SERVER_ERROR)
    {
        source.connect_id_ = 0;
        source.work_thread_id_ = module_logic->get_work_thread_id();
        source.connect_mark_id_ = mark_id;
        source.local_ip_.m_strClientIP = remote_ip;
        source.local_ip_.m_u2Port = remote_port;
    }
    source.type_ = io_type;

    module_logic->do_thread_module_logic(source, recv_packet, send_packet);
}

void CWorkThreadLogic::add_frame_events(uint16 command_id, uint32 mark_id, const std::string& remote_ip, uint16 remote_port, EM_CONNECT_IO_TYPE io_type)
{
    //添加框架通知事件
    App_tms::instance()->AddMessage(0, [this, command_id, mark_id, remote_ip, remote_port, io_type]() {
        do_work_thread_frame_events(command_id, mark_id, remote_ip, remote_port, io_type);
        });
}

void CWorkThreadLogic::add_thread_session(uint32 connect_id, shared_ptr<ISession> session, const _ClientIPInfo& local_info, const _ClientIPInfo& romote_info)
{
    std::lock_guard <std::recursive_mutex> lock(plugin_timer_mutex_);

    //session 建立连接
    uint16 curr_thread_index = connect_id % thread_count_;
    auto module_logic = thread_module_list_[curr_thread_index];

    auto server_id = session->get_mark_id(connect_id);
    if (server_id > 0)
    {
        //关联服务器间链接
        communicate_service_->set_connect_id(server_id, connect_id);
    }

    //必须在IO线程里注册链接信息
    module_logic->add_session(connect_id, session, local_info, romote_info);

    //向插件告知链接建立消息
    App_tms::instance()->AddMessage(curr_thread_index, [session, connect_id, module_logic, local_info, romote_info]() {
        CMessage_Source source;
        auto recv_packet = std::make_shared<CMessage_Packet>();
        auto send_packet = std::make_shared<CMessage_Packet>();

        recv_packet->command_id_ = LOGIC_COMMAND_CONNECT;

        source.connect_id_ = connect_id;
        source.work_thread_id_ = module_logic->get_work_thread_id();
        source.type_ = session->get_io_type();
        source.connect_mark_id_ = session->get_mark_id(connect_id);
        source.local_ip_ = local_info;
        source.remote_ip_ = romote_info;

        module_logic->do_thread_module_logic(source, recv_packet, send_packet);
        });

#ifdef GCOV_TEST
    //测试发送链接建立消息
    if (connect_id == 2)
    {
        auto bridge_packet = std::make_shared<CMessage_Packet>();
        send_io_bridge_message_fail(connect_id, bridge_packet, session);
    }
#endif
}

void CWorkThreadLogic::delete_thread_session(uint32 connect_id, shared_ptr<ISession> session)
{
    std::lock_guard <std::recursive_mutex> lock(plugin_timer_mutex_);

    //session 连接断开
    uint16 curr_thread_index = connect_id % thread_count_;
    auto module_logic = thread_module_list_[curr_thread_index];

    auto server_id = session->get_mark_id(connect_id);
    if (server_id > 0)
    {
        //取消服务器间链接
        communicate_service_->set_connect_id(server_id, 0);
    }

    //回收链接
    module_logic->delete_session_interface(connect_id);

    auto io_type = session->get_io_type();

    //向插件告知链接断开消息
    App_tms::instance()->AddMessage(curr_thread_index, [connect_id, server_id, io_type, module_logic]() {
        CMessage_Source source;
        auto recv_packet = std::make_shared<CMessage_Packet>();
        auto send_packet = std::make_shared<CMessage_Packet>();

        recv_packet->command_id_ = LOGIC_COMMAND_DISCONNECT;

        source.connect_id_ = connect_id;
        source.work_thread_id_ = module_logic->get_work_thread_id();
        source.type_ = io_type;
        source.connect_mark_id_ = server_id;

        module_logic->do_thread_module_logic(source, recv_packet, send_packet);
        });
}

void CWorkThreadLogic::close_session_event(uint32 connect_id, shared_ptr<ISession> session)
{
    //session 关闭事件分发
    uint16 curr_thread_index = connect_id % thread_count_;
    auto module_logic = thread_module_list_[curr_thread_index];

    App_tms::instance()->AddMessage(curr_thread_index, [module_logic, connect_id, session]() {
        if (nullptr != session)
        {
            session->close(connect_id);
        }
        });
}

int CWorkThreadLogic::assignation_thread_module_logic(const uint32 connect_id, const vector<shared_ptr<CMessage_Packet>>& message_list, shared_ptr<ISession> session)
{
    //处理线程的投递
    uint16 curr_thread_index = connect_id % thread_count_;
    auto module_logic = thread_module_list_[curr_thread_index];

#ifdef GCOV_TEST
    PSS_LOGGER_DEBUG("[CWorkThreadLogic::assignation_thread_module_logic]({0}) curr_thread_index={1}， message_list[0]->command_id_={2}).", connect_id, curr_thread_index, message_list[0]->command_id_);
#endif
    //添加到数据队列处理
    App_tms::instance()->AddMessage(curr_thread_index, [this, session, connect_id, message_list, module_logic]() {
        //插件逻辑处理
        do_work_thread_module_logic(session, connect_id, message_list, module_logic);
        });

#ifdef GCOV_TEST
    //测试连接自检
    uint32 check_timeout = 120;
    run_check_task(check_timeout);
#endif
    return 0;
}

int CWorkThreadLogic::assignation_thread_module_logic_iotoio_error(const uint32 connect_id, const vector<shared_ptr<CMessage_Packet>>& message_list, shared_ptr<ISession> session)
{
    uint16 curr_thread_index = connect_id % thread_count_;
    auto module_logic = thread_module_list_[curr_thread_index];

    vector<shared_ptr<CMessage_Packet>> message_list_iotoio;
    for (auto& recv_packet : message_list)
    {
        recv_packet->command_id_ = LOGIC_IOTOIO_CONNECT_NO_EXIST;
        message_list_iotoio.emplace_back(recv_packet);
    }

    App_tms::instance()->AddMessage(curr_thread_index, [this, session, connect_id, message_list_iotoio, module_logic]() {
        do_work_thread_module_logic(session, connect_id, message_list_iotoio, module_logic);
        });

    return 0;
}

int CWorkThreadLogic::assignation_thread_module_logic_with_events(const uint32 connect_id, const vector<shared_ptr<CMessage_Packet>>& message_list, shared_ptr<ISession> session)
{
    //处理线程的投递(内部消息事件的投递)
    uint16 curr_thread_index = connect_id % thread_count_;
    auto module_logic = thread_module_list_[curr_thread_index];

    //添加到数据队列处理
    App_tms::instance()->AddMessage(curr_thread_index, [this, session, connect_id, message_list, module_logic]() {
        do_work_thread_module_logic(session, connect_id, message_list, module_logic);
        });

    return 0;
}

void CWorkThreadLogic::do_work_thread_module_logic(shared_ptr<ISession> session, const uint32 connect_id, const vector<shared_ptr<CMessage_Packet>>& message_list, shared_ptr<CModuleLogic> module_logic) const
{
    CMessage_Source source;
    CMessage_Packet send_packet;

    source.connect_id_ = connect_id;
    source.work_thread_id_ = module_logic->get_work_thread_id();
    source.type_ = session->get_io_type();
    source.connect_mark_id_ = session->get_mark_id(connect_id);

    for (auto recv_packet : message_list)
    {
        auto curr_send_packet = std::make_shared<CMessage_Packet>();
        module_logic->do_thread_module_logic(source, recv_packet, curr_send_packet);

        if (curr_send_packet->buffer_.size() == 0)
        {
            continue;
        }

        auto io_session = module_logic->get_session_interface(connect_id);
        if (nullptr == io_session)
        {
            continue;
        }

        //在这里添加对curr_send_packet的格式化
        if (io_session->is_need_send_format() == true)
        {
            auto curr_format_send_packet = std::make_shared<CMessage_Packet>();
            if(true == io_session->format_send_packet(source.connect_id_, curr_send_packet, curr_format_send_packet))
            {
                //将格式化后的数据填充到send_packet
                send_packet.buffer_.append(curr_format_send_packet->buffer_.c_str(), curr_format_send_packet->buffer_.size());
            }
        }
        else
        {
            //将格式化后的数据填充到send_packet
            send_packet.buffer_.append(curr_send_packet->buffer_.c_str(), curr_send_packet->buffer_.size());
        }
    }

    if (send_packet.buffer_.size() > 0)
    {
        //有需要发送的内容
        session->set_write_buffer(connect_id, send_packet.buffer_.c_str(), send_packet.buffer_.size());
        session->do_write(connect_id);
    }
}

void CWorkThreadLogic::send_io_buffer_to_session(uint32 connect_id, std::shared_ptr<ISession> session, std::shared_ptr<CMessage_Packet> format_packet) const
{
    session->do_write_immediately(connect_id,
        format_packet->buffer_.c_str(),
        format_packet->buffer_.size());
}

void CWorkThreadLogic::do_io_message_delivery(uint32 connect_id, std::shared_ptr<CMessage_Packet> send_packet, shared_ptr<CModuleLogic> module_logic)
{
    auto session = module_logic->get_session_interface(connect_id);
    if (nullptr != session)
    {
        //这里调用格式化发送过程
        if (session->is_need_send_format() == true)
        {
            //需要重新格式化数据
            auto format_packet = std::make_shared<CMessage_Packet>();
            if (false == session->format_send_packet(connect_id, send_packet, format_packet))
            {
                return;
            }

            send_io_buffer_to_session(connect_id, session, format_packet);
        }
        else
        {
            send_io_buffer_to_session(connect_id, session, send_packet);
        }
    }
    else
    {
        //查找是不是服务器间链接，如果是，则调用重连。
        auto server_id = communicate_service_->get_server_id(connect_id);
        if (server_id > 0)
        {
            //重连服务器
            communicate_service_->reset_connect(server_id);
        }
    }
}

void CWorkThreadLogic::do_plugin_thread_module_logic(shared_ptr<CModuleLogic> module_logic, const std::string& message_tag, std::shared_ptr<CMessage_Packet> recv_packet) const
{
    //添加到数据队列处理
    App_tms::instance()->AddMessage(module_logic->get_work_thread_id(), [message_tag, recv_packet, module_logic]() {
        CMessage_Source source;
        auto send_packet = std::make_shared<CMessage_Packet>();

        source.work_thread_id_ = module_logic->get_work_thread_id();
        source.remote_ip_.m_strClientIP = message_tag;
        source.type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_FRAME;

        module_logic->do_thread_module_logic(source, recv_packet, send_packet);

        //内部模块回调不在处理 send_packet 部分。

        });
}

bool CWorkThreadLogic::create_frame_work_thread(uint32 thread_id)
{
    std::lock_guard <std::recursive_mutex> lock(plugin_timer_mutex_);

    if (thread_id < thread_count_)
    {
        PSS_LOGGER_DEBUG("[CWorkThreadLogic::create_frame_work_thread]thread id must more than config thread count.");
        return false;
    }

    if (false == module_init_finish_)
    {
        //如果模块还没全部启动完毕，将这个创建线程的过程，放入vector里面，等模块全部加载完毕，启动。
        plugin_work_thread_buffer_list_.emplace_back(thread_id);
    }
    else
    {
        //查找这个线程ID是否已经存在了
        auto f = plugin_work_thread_list_.find(thread_id);
        if (f != plugin_work_thread_list_.end())
        {
            PSS_LOGGER_DEBUG("[CWorkThreadLogic::create_frame_work_thread]thread id already exist.");
            return false;
        }

        //创建线程
        auto thread_logic = make_shared<CModuleLogic>();

        thread_logic->init_logic(load_module_.get_module_function_list(), (uint16)thread_id);

        plugin_work_thread_list_[thread_id] = thread_logic;

        //初始化线程
        App_tms::instance()->CreateLogic(thread_id);
    }

    return true;
}

bool CWorkThreadLogic::close_frame_work_thread(uint32 thread_id)
{
    std::lock_guard <std::recursive_mutex> lock(plugin_timer_mutex_);

    //不能结束工作线程
    if (thread_id < thread_count_)
    {
        PSS_LOGGER_DEBUG("[CWorkThreadLogic::close_frame_work_thread]thread id must more than config thread count.");
        return false;
    }

    //查找这个线程ID是否已经存在了
    auto f = plugin_work_thread_list_.find(thread_id);
    if (f != plugin_work_thread_list_.end())
    {
        //关闭线程
        f->second->close();
        plugin_work_thread_list_.erase(f);
        return true;
    }

    return false;
}

bool CWorkThreadLogic::delete_frame_message_timer(uint64 timer_id)
{
    std::lock_guard <std::recursive_mutex> lock(plugin_timer_mutex_);

    auto f = plgin_timer_list_.find(timer_id);
    if (f != plgin_timer_list_.end())
    {
        auto timer = f->second;
        auto timer_ptr = timer.lock();
        if (nullptr != timer_ptr)
        {
            timer_ptr->cancel();
        }

        plgin_timer_list_.erase(f);
        return true;
    }
    else
    {
        return false;
    }
}

uint16 CWorkThreadLogic::get_io_work_thread_count() const
{
    return thread_count_;
}

uint16 CWorkThreadLogic::get_plugin_work_thread_count() const
{
    return (uint16)plugin_work_thread_list_.size();
}

int CWorkThreadLogic::module_run(const std::string& module_name, std::shared_ptr<CMessage_Packet> send_packet, std::shared_ptr<CMessage_Packet> return_packet)
{
    return load_module_.plugin_in_name_to_module_run(module_name, send_packet, return_packet);
}

uint32 CWorkThreadLogic::get_curr_thread_logic_id() const
{
    return App_tms::instance()->GetLogicThreadID();
}

bool CWorkThreadLogic::set_io_bridge_connect_id(uint32 from_io_connect_id, uint32 to_io_connect_id)
{
    if (thread_count_ == 0 || thread_module_list_.empty())
    {
        return false;
    }

    auto curr_post_thread_index = from_io_connect_id % thread_count_;
    auto post_module_logic = thread_module_list_[curr_post_thread_index];

    auto session_io = post_module_logic->get_session_interface(from_io_connect_id);
    if (nullptr == session_io)
    {
        //没找到对应链接
        PSS_LOGGER_DEBUG("[CWorkThreadLogic::set_io_bridge_connect_id]from_io_connect_id={} is no find", from_io_connect_id);
        return false;
    }
    else
    {
        //设置端到端的桥接id
        session_io->set_io_bridge_connect_id(from_io_connect_id, to_io_connect_id);
        return true;
    }
}

void CWorkThreadLogic::send_io_bridge_message_fail(uint32 connect_id, std::shared_ptr<CMessage_Packet> bridge_packet, shared_ptr<ISession> session)
{
    vector<std::shared_ptr<CMessage_Packet>> message_error_list;
    bridge_packet->command_id_ = LOGIC_IOTOIO_DATA_ERROR;
    message_error_list.emplace_back(bridge_packet);

    //添加消息处理
    assignation_thread_module_logic(connect_id, message_error_list, session);
}

int CWorkThreadLogic::do_io_bridge_data(uint32 connect_id, uint32 io_bridge_connect_id_, CSessionBuffer& session_recv_buffer, std::size_t length, shared_ptr<ISession> session)
{
    int ret = 0;
    auto bridge_packet = std::make_shared<CMessage_Packet>();
    bridge_packet->buffer_.append(session_recv_buffer.read(), length);
    if (io_bridge_connect_id_ > 0)
    {
        if (false == send_io_bridge_message(io_bridge_connect_id_, bridge_packet))
        {
            //发送失败，将数据包会给业务逻辑去处理
            send_io_bridge_message_fail(connect_id, bridge_packet, session);
            ret = 1;
        }
        else
        {
            ret = 0;
        }
    }
    else
    {
        //发送失败，将数据包会给业务逻辑去处理
        send_io_bridge_message_fail(connect_id, bridge_packet, session);
        ret = 2;
    }

    session_recv_buffer.move(length);
    return ret;
}

void CWorkThreadLogic::send_io_message(uint32 connect_id, std::shared_ptr<CMessage_Packet> send_packet)
{
    //处理线程的投递
    uint16 curr_thread_index = connect_id % thread_count_;
    auto module_logic = thread_module_list_[curr_thread_index];

    //添加到数据队列处理
    App_tms::instance()->AddMessage(curr_thread_index, [this, connect_id, send_packet, module_logic]() {
        do_io_message_delivery(connect_id, send_packet, module_logic);
        });
}

bool CWorkThreadLogic::send_io_bridge_message(uint32 io_bridge_connect_id, std::shared_ptr<CMessage_Packet> send_packet)
{
    uint16 curr_thread_index = io_bridge_connect_id % thread_count_;
    auto module_logic = thread_module_list_[curr_thread_index];

    auto session = module_logic->get_session_interface(io_bridge_connect_id);
    if (nullptr != session)
    {
        session->do_write_immediately(io_bridge_connect_id, send_packet->buffer_.c_str(), send_packet->buffer_.size());
        return true;
    }
    else
    {
        PSS_LOGGER_WARN("[CWorkThreadLogic::send_io_bridge_message]io_bridge_connect_id={0} get_session_interface failed.",io_bridge_connect_id);
        return false;
    }
}

bool CWorkThreadLogic::connect_io_server(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type)
{
    //寻找当前server_id是否存在
    if (true == communicate_service_->is_exist(io_info.server_id))
    {
        PSS_LOGGER_DEBUG("[CWorkThreadLogic::connect_io_server]server_id={0} is exist.",io_info.server_id);
        return false;
    }
    else
    {
        return communicate_service_->add_connect(io_info, io_type);
    }
}

uint32 CWorkThreadLogic::get_connect_id(uint32 server_id) const
{
    //寻找当前server_id是否存在
    if (true == communicate_service_->is_exist(server_id))
    {
        //PSS_LOGGER_DEBUG("[CWorkThreadLogic::get_connect_id]server_id={0} is exist.",server_id);
        return communicate_service_->get_connect_id(server_id);
    }
    else
    {
        PSS_LOGGER_ERROR("[CWorkThreadLogic::get_connect_id]server_id={0} is not exist.",server_id);
        return 0;
    }
}

void CWorkThreadLogic::regedit_bridge_session_id(uint32 connect_id) const
{
    uint16 curr_thread_index = connect_id % thread_count_;
    auto module_logic = thread_module_list_[curr_thread_index];
    auto session = module_logic->get_session_interface(connect_id);
    if (session != nullptr)
    {
        session->regedit_bridge_session_id(connect_id);
    }
}

void CWorkThreadLogic::close_io_server(uint32 server_id)
{
    communicate_service_->close_connect(server_id);
}

uint32 CWorkThreadLogic::get_io_server_id(uint32 connect_id)
{
    return communicate_service_->get_server_id(connect_id);
}

void CWorkThreadLogic::do_work_thread_timeout(uint16 work_thread_id, uint16 last_dispose_command_id, int work_thread_timeout)
{
    //发送消息通知插件
    CMessage_Source source;
    source.work_thread_id_ = work_thread_id;

    auto recv_packet = std::make_shared<CMessage_Packet>();
    auto send_packet = std::make_shared<CMessage_Packet>();

    recv_packet->command_id_ = LOGIC_THREAD_DEAD_LOCK;
    recv_packet->buffer_ = JSON_MODULE_THREAD_ID + std::to_string(work_thread_id)
        + JSON_MODULE_COMMAND_ID + std::to_string(last_dispose_command_id)
        + JSON_MODULE_WORK_THREAD_TIMEOUT + std::to_string(work_thread_timeout) + JSON_MODULE_END;

    thread_module_list_[0]->do_thread_module_logic(source, recv_packet, send_packet);
}

void CWorkThreadLogic::run_check_task(uint32 timeout_seconds)
{
    //检测所有工作线程状态
    for (const auto& module_logic : thread_module_list_)
    {
        auto work_thread_timeout = module_logic->get_work_thread_timeout();
        if (work_thread_timeout > (int)timeout_seconds)
        {
            do_work_thread_timeout(module_logic->get_work_thread_id(), module_logic->get_last_dispose_command_id(), work_thread_timeout);
        }
    }

    //检测所有的tcp链接状态
    if (0 < connect_timeout_)
    {
        uint32 connect_timeout = connect_timeout_;
        for (auto module_logic : thread_module_list_)
        {
            App_tms::instance()->AddMessage(module_logic->get_work_thread_id(), [connect_timeout, module_logic]() {
                module_logic->check_session_io_timeout(connect_timeout);
                });
        }
    }
}

bool CWorkThreadLogic::send_frame_message(uint16 tag_thread_id, const std::string& message_tag, std::shared_ptr<CMessage_Packet> send_packet, CFrame_Message_Delay delay_timer)
{
    if (false == module_init_finish_)
    {
        CDelayPluginMessage plugin_message;
        plugin_message.tag_thread_id_ = tag_thread_id;
        plugin_message.message_tag_ = message_tag;
        plugin_message.send_packet_ = send_packet;
        plugin_message.delay_timer_ = delay_timer;
        plugin_work_thread_buffer_message_list_.emplace_back(plugin_message);
        return true;
    }

    return do_frame_message(tag_thread_id, message_tag, send_packet, delay_timer);
}

bool CWorkThreadLogic::do_frame_message(uint16 tag_thread_id, const std::string& message_tag, std::shared_ptr<CMessage_Packet> send_packet, CFrame_Message_Delay delay_timer)
{
    auto f = plugin_work_thread_list_.find(tag_thread_id);
    if (f == plugin_work_thread_list_.end())
    {
        return false;
    }

    auto plugin_thread = f->second;

    if (delay_timer.delay_seconds_ == std::chrono::seconds(0))
    {
        //不需要延时，立刻投递
        do_plugin_thread_module_logic(plugin_thread, message_tag, send_packet);
    }
    else
    {
        //需要延时，延时后投递
        auto timer_ptr = App_TimerManager::instance()->GetTimerPtr()->addTimer(delay_timer.delay_seconds_, [this, plugin_thread, message_tag, send_packet, delay_timer]()
            {
                //对定时器列表操作加锁
                plugin_timer_mutex_.lock();
                plgin_timer_list_.erase(delay_timer.timer_id_);
                plugin_timer_mutex_.unlock();

                //延时到期，进行投递
                do_plugin_thread_module_logic(plugin_thread, message_tag, send_packet);
            });

        //添加映射关系(只有在定时器ID > 0的时候才能删除)
        if (delay_timer.timer_id_ > 0)
        {
            std::lock_guard <std::recursive_mutex> lock(plugin_timer_mutex_);
            plgin_timer_list_[delay_timer.timer_id_] = timer_ptr;
        }
    }

    return true;
}

bool CWorkThreadLogic::do_work_thread_logic(uint16 tag_thread_id, CFrame_Message_Delay delay_timer, const task_function& func)
{
    auto f = plugin_work_thread_list_.find(tag_thread_id);
    if (f == plugin_work_thread_list_.end())
    {
        return false;
    }

    if (delay_timer.delay_seconds_ == std::chrono::seconds(0))
    {
        //立刻执行线程函数
        App_tms::instance()->AddMessage(tag_thread_id, func);
    }
    else
    {
        //需要延时，延时后投递
        auto timer_ptr = App_TimerManager::instance()->GetTimerPtr()->addTimer(delay_timer.delay_seconds_, [this, tag_thread_id, delay_timer, func]()
            {
                //对定时器列表操作加锁
                plugin_timer_mutex_.lock();
                plgin_timer_list_.erase(delay_timer.timer_id_);
                plugin_timer_mutex_.unlock();

                //延时到期，进行投递
                App_tms::instance()->AddMessage(tag_thread_id, func);
            });

        //添加映射关系(只有在定时器ID > 0的时候才能删除)
        if (delay_timer.timer_id_ > 0)
        {
            std::lock_guard <std::recursive_mutex> lock(plugin_timer_mutex_);
            plgin_timer_list_[delay_timer.timer_id_] = timer_ptr;
        }
    }
    return true;
}

bool CWorkThreadLogic::run_work_thread_logic(uint16 tag_thread_id, CFrame_Message_Delay delay_timer, const task_function& func)
{
    if (false == module_init_finish_)
    {
        auto plugin_func = std::make_shared<CDelayPluginFunc>();
        plugin_func->tag_thread_id_ = tag_thread_id;
        plugin_func->func_          = func;
        plugin_func->delay_timer_   = delay_timer;
        plugin_work_thread_buffer_Func_list_.emplace_back(plugin_func);
        return true;
    }

    return do_work_thread_logic(tag_thread_id, delay_timer, func);
}

shared_ptr<ISession> CWorkThreadLogic::get_session_interface(uint32 connect_id)
{
    uint16 curr_thread_index = connect_id % thread_count_;
    auto module_logic = thread_module_list_[curr_thread_index];

    return module_logic->get_session_interface(connect_id);
}

