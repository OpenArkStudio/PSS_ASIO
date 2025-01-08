#pragma once

#include "define.h"
#include "ISessionService.h"

//插件内需要框架需要使用的对象

//同步异步执行逻辑标记
enum class EM_LOGIC_RUN_STATE
{
    LOGIC_RUN_SYNCHRONOUS = 0,    //同步执行
    LOGIC_RUN_ASYNHRONOUS,        //异步执行
};

class IFrame_Object
{
public:
    virtual bool Regedit_command(uint16 command_id, EM_LOGIC_RUN_STATE em_logic_state = EM_LOGIC_RUN_STATE::LOGIC_RUN_ASYNHRONOUS) = 0;
    virtual bool Regedit_command(uint16 command_id, Logic_message_dispose_fn logic_fn, EM_LOGIC_RUN_STATE em_logic_state = EM_LOGIC_RUN_STATE::LOGIC_RUN_ASYNHRONOUS) = 0;
    virtual ISessionService* get_session_service() = 0;
};