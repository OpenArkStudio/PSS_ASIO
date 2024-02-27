#include "test_Iotoio.h"

int run_test_iotoio()
{
    int ret = 0;
    CIotoIo io_to_io;
    _ClientIPInfo from_io;
    EM_CONNECT_IO_TYPE from_io_type = EM_CONNECT_IO_TYPE::CONNECT_IO_TCP;
    _ClientIPInfo to_io;
    EM_CONNECT_IO_TYPE to_io_type = EM_CONNECT_IO_TYPE::CONNECT_IO_UDP;

    from_io.m_strClientIP = "127.0.0.1";
    from_io.m_u2Port = 10002;
    to_io.m_strClientIP = "127.0.0.1";
    to_io.m_u2Port = 10003;

    io_to_io.add_session_io_mapping(from_io, from_io_type, to_io, to_io_type, ENUM_IO_BRIDGE_TYPE::IO_BRIDGE_BATH);

    io_to_io.delete_session_io_mapping(to_io, to_io_type);

    return 0;
}
