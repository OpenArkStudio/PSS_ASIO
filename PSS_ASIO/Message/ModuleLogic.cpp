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

void CWorkThreadLogic::init_work_thread_logic(int thread_count, command_to_module_function command_to_module_function)
{
    for (int i = 0; i < thread_count; i++)
    {
        auto thread_logic = make_shared<CModuleLogic>();

        thread_logic->init_logic(command_to_module_function);

        thread_module_list_.emplace_back(thread_logic);
    }
}

void CWorkThreadLogic::close()
{
    for (auto f : thread_module_list_)
    {
        f->close();
    }

    thread_module_list_.clear();
}

