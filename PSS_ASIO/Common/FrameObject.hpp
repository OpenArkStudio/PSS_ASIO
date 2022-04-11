#pragma once

#include "IFrameObject.h"
#include "ISessionService.h"

enum class ENUM_LOGIC_COMMAND_TYPE
{
    COMMAND_TYPE_NO_FN = 0,      //不包含处理函数
    COMMAND_TYPE_YES_FN,          //包函处理函数
};

class CLogic_Command_Info
{
public:
    ENUM_LOGIC_COMMAND_TYPE type_ = ENUM_LOGIC_COMMAND_TYPE::COMMAND_TYPE_NO_FN;
    uint16 command_id_ = 0;
    Logic_message_dispose_fn logic_fn_ = nullptr;
};

class CFrame_Object : public IFrame_Object
{
public:
    bool Regedit_command(uint16 command_id) final
    {
        CLogic_Command_Info command_info_;
        command_info_.command_id_ = command_id;
        module_command_list_.emplace_back(command_info_);
        return true;
    };

    bool Regedit_command(uint16 command_id, Logic_message_dispose_fn logic_fn)
    {
        CLogic_Command_Info command_info_;
        command_info_.command_id_ = command_id;
        command_info_.type_ = ENUM_LOGIC_COMMAND_TYPE::COMMAND_TYPE_YES_FN;
        command_info_.logic_fn_ = logic_fn;
        module_command_list_.emplace_back(command_info_);
        return true;
    }

    ISessionService* get_session_service() final
    {
        return session_service_;
    };

    vector<CLogic_Command_Info> module_command_list_;
    ISessionService* session_service_ = nullptr;
};
