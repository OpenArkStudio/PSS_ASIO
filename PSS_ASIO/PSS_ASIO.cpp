// PSS_ASIO.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "ServerService.h"

void test_io_2_io()
{
    CIotoIo io_to_io;
    _ClientIPInfo from_io;
    EM_CONNECT_IO_TYPE from_io_type = EM_CONNECT_IO_TYPE::CONNECT_IO_TCP;
    _ClientIPInfo to_io; 
    EM_CONNECT_IO_TYPE to_io_type = EM_CONNECT_IO_TYPE::CONNECT_IO_UDP;

    from_io.m_strClientIP = "127.0.0.1";
    from_io.m_u2Port = 10002;
    to_io.m_strClientIP = "127.0.0.1";
    to_io.m_u2Port = 10003;

    io_to_io.add_session_io_mapping(from_io, from_io_type, to_io, to_io_type);

    io_to_io.regedit_session_id(from_io, from_io_type, 1);

    cout << "get session id=" << io_to_io.get_to_session_id(1) << endl;

    io_to_io.regedit_session_id(to_io, to_io_type, 2);

    cout << "get session id=" << io_to_io.get_to_session_id(1) << endl;

    io_to_io.delete_session_io_mapping(to_io, to_io_type);

    cout << "get session id=" << io_to_io.get_to_session_id(1) << endl;

    io_to_io.unregedit_session_id(from_io, from_io_type);

    cout << "get session id=" << io_to_io.get_to_session_id(1) << endl;

    getchar();
}

int main()
{
    App_ServerService::instance()->init_servce();
    return 0;
}
