#include "SyncLogic.h"

void CSyncLogic::Close()
{
    command_to_session_function_.clear();
}

void CSyncLogic::Init(const command_to_module_function& command_to_module_function)
{
    command_to_session_function_.clear();
    command_to_session_function_ = command_to_module_function;
}

void CSyncLogic::do_sync_message_list(CMessage_Source& source, vector<std::shared_ptr<CMessage_Packet>>& message_list, std::shared_ptr<CMessage_Packet> send_packet) const
{
    if (command_to_session_function_.size() == 0)
    {
        return;
    }
    else
    {
        //检查是否有需要同步处理的事件
        message_list.erase(
            std::remove_if(
                message_list.begin(),
                message_list.end(),
                [&source, &send_packet](const std::shared_ptr<CMessage_Packet>& packet) {
                    return App_SyncLogic::instance()->do_sync_message(packet->command_id_, source, packet, send_packet);
                }
            ),
            message_list.end());
    }
}

bool CSyncLogic::do_sync_message(const uint16 command_id, const CMessage_Source& source, std::shared_ptr<CMessage_Packet> recv_packet, std::shared_ptr<CMessage_Packet> send_packet)
{
    auto f =command_to_session_function_.find(command_id);
    if (f == command_to_session_function_.end())
    {
        //没找到需要同步执行的命令
        return false;
    }
    else
    {
        //找到了需要同步执行的命令
        f->second(source, recv_packet, send_packet);
        return true;
    }
}
