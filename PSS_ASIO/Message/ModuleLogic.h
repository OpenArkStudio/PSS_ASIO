#pragma once

#include "SessionInterfalce.h"
#include "ModuleInterfalce.h"

//根据线程的逻辑插件处理模块
//add by freeeyes

class CModuleLogic
{
public:
    CModuleLogic() = default;

    void init_logic(command_to_module_function command_to_module_function);

    void add_session(uint32 connect_id, shared_ptr<ISession> session);

    shared_ptr<ISession> get_session_interface(uint32 connect_id);

    void delete_session_interface(uint32 connect_id);

    void close();

private:
    CSessionInterface sessions_interface_;
    CModuleInterface modules_interface_;
};

class CWorkThreadLogic
{
public:
    CWorkThreadLogic() = default;

    void init_work_thread_logic(int thread_count, command_to_module_function command_to_module_function);

    void close();

private:
    vector<shared_ptr<CModuleLogic>> thread_module_list_;
};
