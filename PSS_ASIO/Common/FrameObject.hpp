#pragma once

#include "IFrameObject.h"

class CFrame_Object : public IFrame_Object
{
public:
    void Regedit_command(uint16 command_id) final
    {
        module_command_list_.emplace_back(command_id);
    };

    vector<uint16> module_command_list_;
};
