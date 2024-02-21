#include "Iobridge.h"


bool CIoBridge::add_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type, const _ClientIPInfo& to_io, EM_CONNECT_IO_TYPE to_io_type, ENUM_IO_BRIDGE_TYPE bridge_type)
{
    auto ret = false;
    ret = iotoio_.add_session_io_mapping(from_io, from_io_type, to_io, to_io_type, bridge_type);

    //判断链接是否已存在，如果存在则通知桥接session对象
    auto connect_info = iotoio_.find_io_to_io_list(from_io, from_io_type);
    if (nullptr != connect_info && connect_info->from_session_id_ > 0 && connect_info->to_session_id_ > 0)
    {
        do_bridge_io_2_io(connect_info->from_session_id_, connect_info->to_session_id_, bridge_type);
    }

#ifdef GCOV_TEST
    //测试代码
    do_bridge_io_2_io(100, 200, ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH);
    do_bridge_io_2_io(100, 200, ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_FROM);
    do_bridge_io_2_io(100, 200, ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_TO);
#endif

    return ret;
}

void CIoBridge::do_bridge_io_2_io(uint32 from_session_id, uint32 to_session_id, ENUM_IO_BRIDGE_TYPE bridge_type) const
{
    PSS_LOGGER_DEBUG("[CIoBridge::do_bridge_io_2_io]connect_info->from_session_id:{} connect_info->to_session_id_:{}",
        from_session_id, to_session_id);
    if (bridge_type == ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH)
    {
        //两边的链接已经存在了
        App_WorkThreadLogic::instance()->set_io_bridge_connect_id(from_session_id, to_session_id);
        App_WorkThreadLogic::instance()->set_io_bridge_connect_id(to_session_id, from_session_id);
    }
    else if (bridge_type == ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_FROM)
    {
        App_WorkThreadLogic::instance()->set_io_bridge_connect_id(from_session_id, to_session_id);
    }
    else
    {
        App_WorkThreadLogic::instance()->set_io_bridge_connect_id(to_session_id, from_session_id);
    }
}

bool CIoBridge::delete_session_io_mapping(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE from_io_type)
{
    //查找对应的IO并关闭两端的状态
    auto s_2_s = iotoio_.find_io_to_io_session_info(from_io, from_io_type);
    if (s_2_s.from_session_id_ > 0 && s_2_s.to_session_id_ > 0)
    {
        //将对应链接设置为逻辑模式
        if (s_2_s.bridge_type_ == ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH)
        {
            App_WorkThreadLogic::instance()->set_io_bridge_connect_id(s_2_s.from_session_id_, 0);
            App_WorkThreadLogic::instance()->set_io_bridge_connect_id(s_2_s.to_session_id_, 0);
        }
        else if (s_2_s.bridge_type_ == ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_FROM)
        {
            App_WorkThreadLogic::instance()->set_io_bridge_connect_id(s_2_s.from_session_id_, 0);
        }
        else
        {
            App_WorkThreadLogic::instance()->set_io_bridge_connect_id(s_2_s.to_session_id_, 0);
        }
    }

    return iotoio_.delete_session_io_mapping(from_io, from_io_type);
}

bool CIoBridge::regedit_bridge_session_id(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type, uint32 session_id)
{
    return iotoio_.regedit_bridge_session_id(from_io, io_type, session_id);
}

void CIoBridge::unregedit_bridge_session_id(const _ClientIPInfo& from_io, EM_CONNECT_IO_TYPE io_type)
{
    return iotoio_.unregedit_bridge_session_id(from_io, io_type);
}

Cio_bridge_result CIoBridge::get_to_session_id(uint32 session_id, const _ClientIPInfo& from_io)
{
    return iotoio_.get_to_session_id(session_id, from_io);
}

