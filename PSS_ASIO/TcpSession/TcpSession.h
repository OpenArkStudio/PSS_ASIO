﻿#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include "asio.hpp"

#include "tms.hpp"
#include "SendBuffer.h"
#include "SessionBuffer.hpp"
#include "LoadPacketParse.h"
#include "ISession.h"
#include "ModuleLogic.h"
#include "ConnectCounter.h"

using asio::ip::tcp;

class CTcpSession : public std::enable_shared_from_this<CTcpSession>, public ISession
{
public:
    explicit CTcpSession(tcp::socket socket, asio::io_context* io_context);

    void open(uint32 packet_parse_id, uint32 recv_size);

    _ClientIPInfo get_remote_ip(uint32 connect_id) final;

    void close(uint32 connect_id) final;

    void set_write_buffer(uint32 connect_id, const char* data, size_t length) final;

    void do_write(uint32 connect_id) final;

    void do_write_immediately(uint32 connect_id, const char* data, size_t length) final;

    void add_send_finish_size(uint32 connect_id, size_t send_length) final;

    EM_CONNECT_IO_TYPE get_io_type() final;

    uint32 get_mark_id(uint32 connect_id) final;

    uint32 get_connect_id() final;

    std::chrono::steady_clock::time_point& get_recv_time(uint32 connect_id = 0) final;

    bool format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message) final;

    bool is_need_send_format() final;

    void set_io_bridge_connect_id(uint32 from_io_connect_id, uint32 to_io_connect_id) final;

    void do_read();

    void close_immediaterly();

    void clear_write_buffer();

    void do_read_some(std::error_code ec, std::size_t length);

    void send_write_fail_to_logic(const std::string& write_fail_buffer, std::size_t buffer_length);

    void do_write_finish(const std::error_code& ec, uint32 connect_id, std::shared_ptr<CSendBuffer> send_buffer, std::size_t length);



private:
    tcp::socket socket_;
    asio::io_context* io_context_ = nullptr;
    uint32 connect_id_ = 0;
    uint32 io_bridge_connect_id_ = 0;
    CSessionBuffer session_recv_buffer_;
    std::string session_send_buffer_;
    shared_ptr<_Packet_Parse_Info> packet_parse_interface_ = nullptr;

    std::mutex send_thread_mutex_;
    uint32 io_send_count_ = 0;

    size_t recv_data_size_ = 0;
    size_t send_data_size_ = 0;
    size_t send_buffer_size_ = 0;

    bool is_active_close_ = false;

    _ClientIPInfo remote_ip_;
    _ClientIPInfo local_ip_;
    std::chrono::steady_clock::time_point recv_data_time_ = std::chrono::steady_clock::now();

    EM_CONNECT_IO_TYPE io_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_TCP;
    EM_SESSION_STATE io_state_ = EM_SESSION_STATE::SESSION_IO_LOGIC;
};

