#pragma once

#include "IFrameObject.h"
#include "ISessionService.h"

class CFrame_Object : public IFrame_Object
{
public:
    bool Regedit_command(uint16 command_id) final
    {
        module_command_list_.emplace_back(command_id);
        return true;
    };

    ISessionService* get_session_service() final
    {
        return session_service_;
    };

    vector<uint16> module_command_list_;
    ISessionService* session_service_ = nullptr;
};
