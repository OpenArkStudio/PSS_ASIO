#pragma once

#include <iostream>
#include "define.h"
#include "asio.hpp"
#include "tms.hpp"

#include "SendBuffer.h"
#include "SessionBuffer.hpp"
#include "LoadPacketParse.h"
#include "ConnectCounter.h"
#include "ISession.h"
#include "ModuleLogic.h"

using asio::ip::tcp;

class CTcpClientSession : public std::enable_shared_from_this<CTcpClientSession>, public ISession
{
public:
    CTcpClientSession(asio::io_context& io_context);

    void start(uint32 server_id, uint32 buffer_size, string server_ip, uint16 server_port, uint32 packet_parse_id);

    void do_read();

    void close(uint32 connect_id) final;

    void set_write_buffer(uint32 connect_id, const char* data, size_t length) final; //д��Щ����
    
    void do_write_immediately(uint32 connect_id, const char* data, size_t length) final;

    void do_write(uint32 connect_id) final;

    void add_send_finish_size(uint32 connect_id, size_t send_length) final;

    void clear_write_buffer();

private:
    tcp::socket socket_;
    uint32 server_id_  = 0;
    uint32 connect_id_ = 0;
    CSessionBuffer session_recv_buffer_;
    CSessionBuffer session_send_buffer_;

    shared_ptr<_Packet_Parse_Info> packet_parse_interface_ = nullptr;

    size_t recv_data_size_  = 0;
    size_t send_data_size_  = 0;
};
