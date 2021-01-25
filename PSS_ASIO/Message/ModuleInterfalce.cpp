#include "ModuleInterfalce.h"

void CModuleInterface::copy_from_module_list(command_to_module_function command_to_module_function)
{
    command_to_module_function_.clear();
    command_to_module_function_ = command_to_module_function;
}

int CModuleInterface::do_module_message(const CMessage_Source& source, const CMessage_Packet& recv_packet, CMessage_Packet& send_packet)
{
    auto f = command_to_module_function_.find(recv_packet.command_id_);
    if (f != command_to_module_function_.end())
    {
        //Ö´ÐÐ²å¼þº¯Êý
        return f->second(source, recv_packet, send_packet);
    }
    else
    {
        PSS_LOGGER_DEBUG("[CModuleInterface::do_module_message]no find command_id({0})", source.connect_id_);
        return -1;
    }
}

void CModuleInterface::close()
{
    command_to_module_function_.clear();
}
