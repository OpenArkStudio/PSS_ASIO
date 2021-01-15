#pragma once

//处理消息和投递的类

#include "define.h"
#include "ISession.h"

class CMessageInterface
{
private:
    CMessageInterface() = default;

    void add_session_interface(uint32 connect_id, shared_ptr<ISession> session);

    shared_ptr<ISession> get_session_interface(uint32 connect_id);

    void delete_session_interface(uint32 connect_id);

private:
    using hashmapsessions = unordered_map<uint32, shared_ptr<ISession>>;
    hashmapsessions sessions_list_;
};