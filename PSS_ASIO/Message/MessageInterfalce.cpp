#include "MessageInterfalce.h"

void CMessageInterface::add_session_interface(uint32 connect_id, shared_ptr<ISession> session)
{
    auto f = sessions_list_.find(connect_id);
    if (f == sessions_list_.end())
    {
        sessions_list_[connect_id] = session;
    }
}

shared_ptr<ISession> CMessageInterface::get_session_interface(uint32 connect_id)
{
    auto f = sessions_list_.find(connect_id);
    if (f != sessions_list_.end())
    {
        return f->second;
    }
    else
    {
        return nullptr;
    }
}

void CMessageInterface::delete_session_interface(uint32 connect_id)
{
    sessions_list_.erase(connect_id);
}
