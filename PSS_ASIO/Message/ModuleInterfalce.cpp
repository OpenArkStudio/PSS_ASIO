#include "ModuleInterfalce.h"

void CModuleInterface::copy_from_module_list(command_to_module_function command_to_module_function)
{
    command_to_module_function_.clear();
    command_to_module_function_ = command_to_module_function;
}

void CModuleInterface::do_module_message(uint16 command_id, CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    auto f = command_to_module_function_.find(command_id);
    if (f != command_to_module_function_.end())
    {
        //Ö´ÐÐ²å¼þº¯Êý
        f->second(command_id, recv_packet, send_packet);
    }
}

void CModuleInterface::close()
{
    command_to_module_function_.clear();
}
