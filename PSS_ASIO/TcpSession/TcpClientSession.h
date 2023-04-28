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
    explicit CTcpClientSession(asio::io_context* io_context);

    bool start(const CConnect_IO_Info& io_info);

    void do_read();

    _ClientIPInfo get_remote_ip(uint32 connect_id) final;

    void close(uint32 connect_id) final;

    void set_write_buffer(uint32 connect_id, const char* data, size_t length) final; //写入些缓冲
    
    void do_write_immediately(uint32 connect_id, const char* data, size_t length) final;

    void do_write(uint32 connect_id) final;

    void add_send_finish_size(uint32 connect_id, size_t send_length) final;

    EM_CONNECT_IO_TYPE get_io_type() final;

    uint32 get_mark_id(uint32 connect_id) final;

    std::chrono::steady_clock::time_point& get_recv_time() final;

    bool format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message) final;

    bool is_need_send_format() final;

    bool is_connect() final;

    void clear_write_buffer();

    void do_read_some(std::error_code ec, std::size_t length);

    void handle_connect(const asio::error_code& ec, tcp::resolver::results_type::iterator endpoint_iter);

    void send_write_fail_to_logic(const std::string& write_fail_buffer, std::size_t buffer_length);

private:
    tcp::socket socket_;
    asio::io_context* io_context_ = nullptr;
    uint32 server_id_  = 0;
    uint32 connect_id_ = 0;
    bool is_connect_ = false;
    CSessionBuffer session_recv_buffer_;
    CSessionBuffer session_send_buffer_;

    _ClientIPInfo remote_ip_;
    _ClientIPInfo local_ip_;

    shared_ptr<_Packet_Parse_Info> packet_parse_interface_ = nullptr;

    size_t recv_data_size_  = 0;
    size_t send_data_size_  = 0;
    std::chrono::steady_clock::time_point recv_data_time_ = std::chrono::steady_clock::now();

    EM_CONNECT_IO_TYPE io_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_SERVER_TCP;
};
