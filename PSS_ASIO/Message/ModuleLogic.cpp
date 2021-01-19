#include "ModuleLogic.h"

void CModuleLogic::init_logic(command_to_module_function command_to_module_function)
{
    modules_interface_.copy_from_module_list(command_to_module_function);
}

void CModuleLogic::add_session(uint32 connect_id, shared_ptr<ISession> session)
{
    sessions_interface_.add_session_interface(connect_id, session);
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

int CModuleLogic::do_thread_module_logic(CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    return modules_interface_.do_module_message(recv_packet.command_id_, recv_packet, send_packet);
}

void CWorkThreadLogic::init_work_thread_logic(int thread_count)
{
    //初始化线程数
    thread_count_ = thread_count;

    App_tms::instance()->Init();

    //初始化插件加载
    load_module_.load_plugin_module("./", "Test_Logic.dll", "test param");

    //执行线程对应创建
    for (int i = 0; i < thread_count; i++)
    {
        auto thread_logic = make_shared<CModuleLogic>();

        thread_logic->init_logic(load_module_.get_module_function_list());

        thread_module_list_.emplace_back(thread_logic);

        //初始化线程
        App_tms::instance()->CreateLogic(i);
    }
    
}

void CWorkThreadLogic::close()
{
    //关闭线程操作
    App_tms::instance()->Close();

    for (auto f : thread_module_list_)
    {
        f->close();
    }

    thread_module_list_.clear();

    //关闭模板操作
    load_module_.Close();
}

void CWorkThreadLogic::add_thread_session(uint32 connect_id, shared_ptr<ISession> session)
{
    //session 建立连接
    uint16 curr_thread_index = connect_id % thread_count_;

    thread_module_list_[curr_thread_index]->add_session(connect_id, session);
}

void CWorkThreadLogic::delete_thread_session(uint32 connect_id)
{
    //session 连接断开
    uint16 curr_thread_index = connect_id % thread_count_;

    thread_module_list_[curr_thread_index]->delete_session_interface(connect_id);
}

void CWorkThreadLogic::close_session_event(uint32 connect_id)
{
    //session 关闭事件分发
    uint16 curr_thread_index = connect_id % thread_count_;

    auto session = thread_module_list_[curr_thread_index]->get_session_interface(connect_id);

    App_tms::instance()->AddMessage(curr_thread_index, [session, connect_id]() {
        session->Close(connect_id);
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
        CMessage_Packet send_packet;
        for (auto recv_packet : message_list)
        {
            module_logic->do_thread_module_logic(recv_packet, send_packet);
        }

        session->set_write_buffer(connect_id, send_packet.buffer_.c_str(), send_packet.buffer_.size());
        session->do_write(connect_id);
        });

    return 0;
}

