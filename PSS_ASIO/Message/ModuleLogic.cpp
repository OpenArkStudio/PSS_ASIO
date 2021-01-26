#include "ModuleLogic.h"

void CModuleLogic::init_logic(command_to_module_function command_to_module_function, uint16 work_thread_id)
{
    modules_interface_.copy_from_module_list(command_to_module_function);
    work_thread_id_ = work_thread_id;
}

void CModuleLogic::add_session(uint32 connect_id, shared_ptr<ISession> session, const _ClientIPInfo& local_info, const _ClientIPInfo& romote_info)
{
    sessions_interface_.add_session_interface(connect_id, session, local_info, romote_info);
}

shared_ptr<ISession> CModuleLogic::get_session_interface(uint32 connect_id)
{
    return sessions_interface_.get_session_interface(connect_id);
}

void CModuleLogic::delete_session_interface(uint32 connect_id)
{
    sessions_interface_.delete_session_interface(connect_id);
}

void CModuleLogic::close()
{
    modules_interface_.close();
    sessions_interface_.close();
}

int CModuleLogic::do_thread_module_logic(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    return modules_interface_.do_module_message(source, recv_packet, send_packet);
}

uint16 CModuleLogic::get_work_thread_id()
{
    return work_thread_id_;
}

void CWorkThreadLogic::init_work_thread_logic(int thread_count, config_logic_list& logic_list, ISessionService* session_service)
{
    //初始化线程数
    thread_count_ = thread_count;

    App_tms::instance()->Init();

    load_module_.set_session_service(session_service);

    //初始化插件加载
    for (auto logic_library : logic_list)
    {
        load_module_.load_plugin_module(logic_library.logic_path_, 
            logic_library.logic_file_name_, 
            logic_library.logic_param_);
    }

    //执行线程对应创建
    for (int i = 0; i < thread_count; i++)
    {
        auto thread_logic = make_shared<CModuleLogic>();

        thread_logic->init_logic(load_module_.get_module_function_list(), i);

        thread_module_list_.emplace_back(thread_logic);

        //初始化线程
        App_tms::instance()->CreateLogic(i);
    }
    
}

void CWorkThreadLogic::init_communication_service(ICommunicationInterface* communicate_service)
{
    communicate_service_ = communicate_service;
}

void CWorkThreadLogic::close()
{
    //关闭线程操作
    App_tms::instance()->Close();

    communicate_service_->close();

    for (auto f : thread_module_list_)
    {
        f->close();
    }

    thread_module_list_.clear();

    //关闭模板操作
    load_module_.Close();
}

void CWorkThreadLogic::add_thread_session(uint32 connect_id, shared_ptr<ISession> session, _ClientIPInfo& local_info, const _ClientIPInfo& romote_info)
{
    //session 建立连接
    uint16 curr_thread_index = connect_id % thread_count_;

    thread_module_list_[curr_thread_index]->add_session(connect_id, session, local_info, romote_info);

    auto server_id = session->get_mark_id(connect_id);
    if (server_id > 0)
    {
        //关联服务器间链接
        communicate_service_->set_connect_id(server_id, connect_id);
    }
}

void CWorkThreadLogic::delete_thread_session(uint32 connect_id, shared_ptr<ISession> session)
{
    //session 连接断开
    uint16 curr_thread_index = connect_id % thread_count_;

    thread_module_list_[curr_thread_index]->delete_session_interface(connect_id);

    auto server_id = session->get_mark_id(connect_id);
    if (server_id > 0)
    {
        //取消服务器间链接
        communicate_service_->set_connect_id(server_id, 0);
    }
}

void CWorkThreadLogic::close_session_event(uint32 connect_id)
{
    //session 关闭事件分发
    uint16 curr_thread_index = connect_id % thread_count_;

    auto session = thread_module_list_[curr_thread_index]->get_session_interface(connect_id);

    App_tms::instance()->AddMessage(curr_thread_index, [session, connect_id]() {
        session->close(connect_id);
        });
}

int CWorkThreadLogic::do_thread_module_logic(const uint32 connect_id, vector<CMessage_Packet>& message_list, shared_ptr<ISession> session)
{
    //处理线程的投递
    uint16 curr_thread_index = connect_id % thread_count_;
    auto module_logic = thread_module_list_[curr_thread_index];

    //添加到数据队列处理
    App_tms::instance()->AddMessage(curr_thread_index, [session, connect_id, message_list, module_logic]() {
        //PSS_LOGGER_DEBUG("[CTcpSession::AddMessage]count={}.", message_list.size());
        CMessage_Source source;
        CMessage_Packet send_packet;

        source.connect_id_ = connect_id;
        source.work_thread_id_ = module_logic->get_work_thread_id();
        source.type_ = session->get_io_type();
        source.connect_mark_id_ = session->get_mark_id(connect_id);

        for (auto recv_packet : message_list)
        {
            module_logic->do_thread_module_logic(source, recv_packet, send_packet);
        }

        session->set_write_buffer(connect_id, send_packet.buffer_.c_str(), send_packet.buffer_.size());
        session->do_write(connect_id);
        });

    return 0;
}

void CWorkThreadLogic::send_io_message(uint32 connect_id, CMessage_Packet send_packet)
{
    //处理线程的投递
    uint16 curr_thread_index = connect_id % thread_count_;
    auto module_logic = thread_module_list_[curr_thread_index];

    //添加到数据队列处理
    App_tms::instance()->AddMessage(curr_thread_index, [connect_id, send_packet, module_logic]() {
        module_logic->get_session_interface(connect_id)->do_write_immediately(connect_id,
            send_packet.buffer_.c_str(),
            send_packet.buffer_.size());
        });
}

bool CWorkThreadLogic::connect_io_server(const CConnect_IO_Info& io_info, EM_CONNECT_IO_TYPE io_type)
{
    //寻找当前server_id是否存在
    if (true == communicate_service_->is_exist(io_info.server_id))
    {
        PSS_LOGGER_DEBUG("[CWorkThreadLogic::connect_io_server]server_id={0} is exist.");
        return false;
    }
    else
    {
        return communicate_service_->add_connect(io_info, io_type);
    }
}

void CWorkThreadLogic::close_io_server(uint32 server_id)
{
    communicate_service_->close_connect(server_id);
}

