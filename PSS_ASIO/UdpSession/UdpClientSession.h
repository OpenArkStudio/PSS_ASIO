#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include "asio.hpp"
#include "tms.hpp"

#include "SendBuffer.h"
#include "SessionBuffer.hpp"
#include "LoadPacketParse.h"
#include "ConnectCounter.h"
#include "ISession.h"
#include "ModuleLogic.h"

using asio::ip::udp;

class CUdpClientSession : public std::enable_shared_from_this<CUdpClientSession>, public ISession
{
public:
    CUdpClientSession(asio::io_context* io_context);

    void start(const CConnect_IO_Info& io_type);

    void do_receive();

    void clear_write_buffer(size_t length);

    void close(uint32 connect_id) final;

    void do_write(uint32 connect_id) final;

    void set_write_buffer(uint32 connect_id, const char* data, size_t length) final;

    void add_send_finish_size(uint32 connect_id, size_t send_length) final;

    void do_write_immediately(uint32 connect_id, const char* data, size_t length) final;

    EM_CONNECT_IO_TYPE get_io_type() final;

    uint32 get_mark_id(uint32 connect_id);

private:
    udp::socket socket_;
    uint32 server_id_  = 0;
    uint32 connect_id_ = 0;
    CSessionBuffer session_recv_buffer_;
    CSessionBuffer session_send_buffer_;
    udp::endpoint recv_endpoint_;
    udp::endpoint send_endpoint_;

    shared_ptr<_Packet_Parse_Info> packet_parse_interface_ = nullptr;

    size_t recv_data_size_ = 0;
    size_t send_data_size_ = 0;

    EM_CONNECT_IO_TYPE io_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_SERVER_UDP;
};

