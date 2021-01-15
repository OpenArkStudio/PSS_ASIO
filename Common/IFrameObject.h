#pragma once

#include "define.h"

//插件内需要框架需要使用的对象

class IFrame_Object
{
public:
    virtual void Regedit_command(uint16 command_id) = 0;
};