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
#include "Iobridge.h"

using asio::ip::udp;

class CUdpClientSession : public std::enable_shared_from_this<CUdpClientSession>, public ISession
{
public:
    explicit CUdpClientSession(asio::io_context* io_context);

    void start(const CConnect_IO_Info& io_type);

    void do_receive();

    void clear_write_buffer(size_t length);

    _ClientIPInfo get_remote_ip(uint32 connect_id) final;

    void close(uint32 connect_id) final;

    void do_write(uint32 connect_id) final;

    void set_write_buffer(uint32 connect_id, const char* data, size_t length) final;

    void add_send_finish_size(uint32 connect_id, size_t send_length) final;

    void do_write_immediately(uint32 connect_id, const char* data, size_t length) final;

    EM_CONNECT_IO_TYPE get_io_type() final;

    std::chrono::steady_clock::time_point& get_recv_time(uint32 connect_id) final;

    bool format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message) final;

    bool is_need_send_format() final;

    void send_io_data(uint32 connect_id, std::shared_ptr<CSendBuffer> send_buffer);

    uint32 get_mark_id(uint32 connect_id) final;

    void set_io_bridge_connect_id(uint32 from_io_connect_id, uint32 to_io_connect_id) final;

    void do_receive_from(std::error_code ec, std::size_t length);

private:
    udp::socket socket_;
    asio::io_context* io_context_;
    uint32 server_id_  = 0;
    uint32 connect_id_ = 0;
    uint32 io_bradge_connect_id_ = 0;
    CSessionBuffer session_recv_buffer_;
    CSessionBuffer session_send_buffer_;
    udp::endpoint recv_endpoint_;
    udp::endpoint send_endpoint_;

    shared_ptr<_Packet_Parse_Info> packet_parse_interface_ = nullptr;

    size_t recv_data_size_ = 0;
    size_t send_data_size_ = 0;
    std::chrono::steady_clock::time_point recv_data_time_ = std::chrono::steady_clock::now();

    EM_CONNECT_IO_TYPE io_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_SERVER_UDP;
    EM_SESSION_STATE io_state_ = EM_SESSION_STATE::SESSION_IO_LOGIC;
};

