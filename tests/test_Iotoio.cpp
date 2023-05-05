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
    io_to_io.regedit_session_id(from_io, from_io_type, 1);
    if (io_to_io.get_to_session_id(1, from_io) != 0)
    {
        return 1;
    }

    io_to_io.regedit_session_id(to_io, to_io_type, 2);
    if (io_to_io.get_to_session_id(1, from_io) != 2)
    {
        return 1;
    }

    io_to_io.delete_session_io_mapping(to_io, to_io_type);
    if (io_to_io.get_to_session_id(1, from_io) != 0)
    {
        return 1;
    }

    io_to_io.unregedit_session_id(from_io, from_io_type);
    if (io_to_io.get_to_session_id(1, from_io) != 0)
    {
        return 1;
    }

    return 0;
}
