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
#include "Iobridge.h"
#include "IoListManager.h"

using asio::ip::udp;

enum class EM_UDP_VALID
{
    UDP_INVALUD = 0,
    UDP_VALUD,
};

const uint32 udp_net_buffer_size = 10 * 1024 * 1024;

class CUdp_Session_Info
{
public:
    uint32 connect_id_ = 0;
    uint32 io_bridge_connect_id_ = 0;
    EM_SESSION_STATE io_state_ = EM_SESSION_STATE::SESSION_IO_LOGIC;
    udp::endpoint send_endpoint;
    size_t recv_data_size_ = 0;
    size_t send_data_size_ = 0;
    CSessionBuffer session_send_buffer_;
    EM_UDP_VALID udp_state = EM_UDP_VALID::UDP_INVALUD;
};

class CUdpServer : public std::enable_shared_from_this<CUdpServer>, public ISession, public CIo_Net_server
{
public:
    CUdpServer(asio::io_context* io_context, const std::string& server_ip, io_port_type port, uint32 packet_parse_id, uint32 max_recv_size, uint32 max_send_size, EM_NET_TYPE em_net_type, CIo_List_Manager* io_list_manager);

    ~CUdpServer() = default;

    void start();

    _ClientIPInfo get_remote_ip(uint32 connect_id) final;

    void close(uint32 connect_id) final;

    void close() final;

    void set_write_buffer(uint32 connect_id, const char* data, size_t length) final;

    void do_write(uint32 connect_id) final;

    void do_write_immediately(uint32 connect_id, const char* data, size_t length) final;

    void add_send_finish_size(uint32 connect_id, size_t length) final;

    EM_CONNECT_IO_TYPE get_io_type() final;

    uint32 get_mark_id(uint32 connect_id) final;

    uint32 get_connect_id() final;

    void regedit_bridge_session_id(uint32 connect_id = 0) final;

    std::chrono::steady_clock::time_point& get_recv_time(uint32 connect_id = 0) final;

    bool format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message) final;

    bool is_need_send_format() final;

    void set_io_bridge_connect_id(uint32 from_io_connect_id, uint32 to_io_connect_id) final;

private:
    void close_server();

    void do_receive();

    void do_receive_from(std::error_code ec, std::size_t length);

    void clear_write_buffer(shared_ptr<CUdp_Session_Info> session_info) const;

    uint32 add_udp_endpoint(const udp::endpoint& recv_endpoint_, size_t length, uint32 max_buffer_length);

    shared_ptr<CUdp_Session_Info> find_udp_endpoint_by_id(uint32 connect_id);
   
    void close_udp_endpoint_by_id(uint32 connect_id);

    udp::socket socket_;
    uint32 connect_client_id_ = 0;
    udp::endpoint recv_endpoint_;
    
    using mapudpid2endpoint = map<uint32, shared_ptr<CUdp_Session_Info>>;
    using mapudpendpoint2id = map<udp::endpoint, uint32>;
    mapudpid2endpoint udp_id_2_endpoint_list_;
    mapudpendpoint2id udp_endpoint_2_id_list_;

    uint32 max_recv_size_ = 0;
    uint32 max_send_size_ = 0;
    asio::io_context* io_context_ = nullptr;
    std::chrono::steady_clock::time_point recv_data_time_ = std::chrono::steady_clock::now();

    CSessionBuffer session_recv_buffer_;
    shared_ptr<_Packet_Parse_Info> packet_parse_interface_ = nullptr;

    EM_CONNECT_IO_TYPE io_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_UDP;

    using hashmapcid_recv_data_time = unordered_map<uint32, std::chrono::steady_clock::time_point>;
    hashmapcid_recv_data_time cid_recv_data_time_;

    string server_ip_;
    io_port_type server_port_;
    CIo_List_Manager* io_list_manager_ = nullptr;
};

