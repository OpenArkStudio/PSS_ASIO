#pragma once

#include "IFrameObject.h"
#include "ISessionService.h"

class CFrame_Object : public IFrame_Object
{
public:
    bool Regedit_command(uint16 command_id) final
    {
        if (command_id <= LOGIC_MAX_FRAME_COMMAND)
        {
            PSS_LOGGER_INFO("[CFrame_Object::Regedit_command]command_id must more than {0}.", LOGIC_MAX_FRAME_COMMAND);
            return false;
        }
        else
        {
            module_command_list_.emplace_back(command_id);
            return true;
        }
    };

    ISessionService* get_session_service() final
    {
        return session_service_;
    };

    vector<uint16> module_command_list_;
    ISessionService* session_service_ = nullptr;
};
