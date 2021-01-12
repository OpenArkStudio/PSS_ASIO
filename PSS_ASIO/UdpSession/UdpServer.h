#pragma once

#include <iostream>
#include "define.h"
#include "asio.hpp"
#include "tms.hpp"

#include "SendBuffer.h"
#include "SessionBuffer.hpp"
#include "LoadPacketParse.h"
#include "ConnectCounter.h"

using asio::ip::udp;

enum class EM_UDP_VALID
{
    UDP_INVALUD = 0,
    UDP_VALUD,
};

class CUdp_Session_Info
{
public:
    udp::endpoint send_endpoint;
    size_t recv_data_size_ = 0;
    size_t send_data_size_ = 0;
    EM_UDP_VALID udp_state = EM_UDP_VALID::UDP_INVALUD;
};

class CUdpServer : public std::enable_shared_from_this<CUdpServer>
{
public:
    CUdpServer(asio::io_context& io_context, std::string server_ip, short port, uint32 packet_parse_id, uint32 max_buffer_length);

private:
    void do_receive();

    void set_write_buffer(const char* data, size_t length);

    void clear_write_buffer();

    void do_write(uint32 connect_id);

    uint32 add_udp_endpoint(udp::endpoint recv_endpoint_, size_t length);

    CUdp_Session_Info find_udp_endpoint_by_id(uint32 connect_id);

    void write_send_buffer_size(uint32 connect_id, size_t length);

    udp::socket socket_;
    uint32 connect_client_id_ = 0;
    uint32 packet_parse_id_ = 0;
    udp::endpoint recv_endpoint_;
    
    using mapudpid2endpoint = map<uint32, CUdp_Session_Info>;
    using mapudpendpoint2id = map<udp::endpoint, uint32>;
    mapudpid2endpoint udp_id_2_endpoint_list;
    mapudpendpoint2id udp_endpoint_2_id_list;

    CSessionBuffer session_recv_buffer_;
    CSessionBuffer session_send_buffer_;
    shared_ptr<_Packet_Parse_Info> packet_parse_interface_ = nullptr;
};

