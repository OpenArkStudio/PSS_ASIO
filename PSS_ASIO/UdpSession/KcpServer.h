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
#include "kcp/ikcp.h"

using asio::ip::udp;

//实现KCP的服务端代码
//add by freeeyes

enum class EM_KCP_VALID
{
    KCP_INVALUD = 0,
    KCP_VALUD,
};

//kcp的发送数据对象，回调参数
class CKcp_send_info
{
public:
    uint32 connecy_id_;
    udp::endpoint send_endpoint;
};

class CKcp_Session_Info
{
public:
    udp::endpoint send_endpoint;
    size_t recv_data_size_ = 0;
    size_t send_data_size_ = 0;
    CSessionBuffer session_send_buffer_;
    EM_KCP_VALID udp_state = EM_KCP_VALID::KCP_INVALUD;
};

class CKcpServer : public std::enable_shared_from_this<CKcpServer>, public ISession
{
public:
    CKcpServer(asio::io_context& io_context, const std::string& server_ip, short port, uint32 packet_parse_id, uint32 max_recv_size, uint32 max_send_size, uint32 kcp_id);

    void start();

    void close(uint32 connect_id) final;

    void set_write_buffer(uint32 connect_id, const char* data, size_t length) final;

    void do_write(uint32 connect_id) final;

    void do_write_immediately(uint32 connect_id, const char* data, size_t length) final;

    void add_send_finish_size(uint32 connect_id, size_t length) final;

    void send_io_data_to_point(const char* data, size_t length);

    EM_CONNECT_IO_TYPE get_io_type() final;

    uint32 get_mark_id(uint32 connect_id) final;

    std::chrono::steady_clock::time_point& get_recv_time() final;

    bool format_send_packet(uint32 connect_id, std::shared_ptr<CMessage_Packet> message, std::shared_ptr<CMessage_Packet> format_message) final;

    bool is_need_send_format() final;

private:
    void do_receive();

    void do_receive_from(std::error_code ec, std::size_t length);

    void clear_write_buffer(shared_ptr<CKcp_Session_Info> session_info) const;

    uint32 add_udp_endpoint(const udp::endpoint& recv_endpoint_, size_t length, uint32 max_buffer_length);

    shared_ptr<CKcp_Session_Info> find_udp_endpoint_by_id(uint32 connect_id);
   
    void close_udp_endpoint_by_id(uint32 connect_id);

    void set_kcp_send_info(uint32 connect_id, udp::endpoint kcp_send_endpoint);

    udp::socket socket_;
    uint32 connect_client_id_ = 0;
    udp::endpoint recv_endpoint_;
    
    using mapudpid2endpoint = map<uint32, shared_ptr<CKcp_Session_Info>>;
    using mapudpendpoint2id = map<udp::endpoint, uint32>;
    mapudpid2endpoint udp_id_2_endpoint_list_;
    mapudpendpoint2id udp_endpoint_2_id_list_;

    uint32 max_recv_size_ = 0;
    uint32 max_send_size_ = 0;
    asio::io_context* io_context_ = nullptr;
    std::chrono::steady_clock::time_point recv_data_time_ = std::chrono::steady_clock::now();

    CSessionBuffer session_recv_buffer_;       //存储kcp原始包
    CSessionBuffer session_recv_data_buffer_;  //解析出来的kcp内容包
    shared_ptr<_Packet_Parse_Info> packet_parse_interface_ = nullptr;

    EM_CONNECT_IO_TYPE io_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_KCP;

    uint32 kcp_id_ = 0;
    ikcpcb* kcpcb_ = nullptr;
    CKcp_send_info kcp_send_info_;
};

