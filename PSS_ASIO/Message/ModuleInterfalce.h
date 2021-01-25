#pragma once

//处理消息和投递的类

#include "define.h"
#include "ISession.h"
#include "LoadModule.h"

class CModuleInterface
{
public:
    CModuleInterface() = default;

    void copy_from_module_list(command_to_module_function command_to_module_function);

    int do_module_message(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet);

    void close();

private:
    command_to_module_function command_to_module_function_;
};