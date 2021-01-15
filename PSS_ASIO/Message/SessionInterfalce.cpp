#include "SessionInterfalce.h"

void CSessionInterface::add_session_interface(uint32 connect_id, shared_ptr<ISession> session)
{
    auto f = sessions_list_.find(connect_id);
    if (f == sessions_list_.end())
    {
        sessions_list_[connect_id] = session;
    }
}

shared_ptr<ISession> CSessionInterface::get_session_interface(uint32 connect_id)
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

void CSessionInterface::delete_session_interface(uint32 connect_id)
{
    sessions_list_.erase(connect_id);
}

void CSessionInterface::close()
{
    sessions_list_.clear();
}
