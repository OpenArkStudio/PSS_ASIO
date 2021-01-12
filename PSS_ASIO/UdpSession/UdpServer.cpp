#include "UdpServer.h"

CUdpServer::CUdpServer(asio::io_context& io_context, std::string server_ip, short port, uint32 packet_parse_id, uint32 max_buffer_length)
    : socket_(io_context, udp::endpoint(asio::ip::address_v4::from_string(server_ip), port)), packet_parse_id_(packet_parse_id)
{
    //处理链接建立消息
    std::cout << "[CUdpServer::do_accept](" << socket_.local_endpoint() << ") Begin Accept." << std::endl;

    session_recv_buffer_.Init(max_buffer_length);
    session_send_buffer_.Init(max_buffer_length);

    packet_parse_interface_ = App_PacketParseLoader::instance()->GetPacketParseInfo(packet_parse_id);

    do_receive();
}

void CUdpServer::do_receive()
{
    socket_.async_receive_from(
        asio::buffer(session_recv_buffer_.get_curr_write_ptr(), session_recv_buffer_.get_buffer_size()), recv_endpoint_,
        [this](std::error_code ec, std::size_t length)
        {
            //查询当前的connect_id
            auto connect_id = add_udp_endpoint(recv_endpoint_);
            
            if (!ec && length > 0)
            {
                //处理数据包
                auto self(shared_from_this());

                //如果缓冲已满，断开连接，不再接受数据。
                if (session_recv_buffer_.get_buffer_size() == 0)
                {
                    //链接断开(缓冲撑满了)
                    session_recv_buffer_.move(length);
                    do_receive();
                }

                //处理数据拆包
                vector<CMessage_Packet> message_list;
                bool ret = packet_parse_interface_->Parse_Packet_From_Recv_Buffer(connect_client_id_, &session_recv_buffer_, message_list, EM_CONNECT_IO_TYPE::CONNECT_IO_UDP);
                if (!ret)
                {
                    //链接断开(解析包不正确)
                    session_recv_buffer_.move(length);
                    do_receive();
                }

                //添加到数据队列处理
                App_tms::instance()->AddMessage(1, [self, message_list, connect_id]() {
                    PSS_LOGGER_DEBUG("[CTcpSession::AddMessage]count={}.", message_list.size());
                    for (auto packet : message_list)
                    {
                        self->set_write_buffer(packet.head_.c_str(), packet.head_.size());
                        self->set_write_buffer(packet.body_.c_str(), packet.body_.size());
                    }

                    self->do_write(connect_id);
                    });


                do_receive();
            }
            else
            {
                do_receive();
            }
        });
}

void CUdpServer::set_write_buffer(const char* data, size_t length)
{
    if (session_send_buffer_.get_buffer_size() <= length)
    {
        //发送些缓冲已经满了
        return;
    }

    std::memcpy(session_send_buffer_.get_curr_write_ptr(),
        data,
        length);
    session_send_buffer_.set_write_data(length);
}

void CUdpServer::clear_write_buffer()
{
    session_send_buffer_.move(session_send_buffer_.get_write_size());
}

void CUdpServer::do_write(uint32 connect_id)
{
    auto sender_endpoint = find_udp_endpoint_by_id(connect_id);

    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(session_send_buffer_.read(), session_send_buffer_.get_write_size());
    send_buffer->buffer_length_ = session_send_buffer_.get_write_size();

    //PSS_LOGGER_DEBUG("[CUdpServer::do_write]send_buffer->buffer_length_={}.", send_buffer->buffer_length_);
    clear_write_buffer();

    socket_.async_send_to(
        asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_), sender_endpoint,
        [this](std::error_code /*ec*/, std::size_t /*bytes_sent*/)
        {
            do_receive();
        });
}

uint32 CUdpServer::add_udp_endpoint(udp::endpoint recv_endpoint_)
{
    auto f = udp_endpoint_2_id_list.find(recv_endpoint_);
    if (f != udp_endpoint_2_id_list.end())
    {
        //找到了，返回ID
        return f->second;
    }
    else
    {
        //生成一个新的ID
        auto connect_id = App_ConnectCounter::instance()->CreateCounter();
        udp_endpoint_2_id_list[recv_endpoint_] = connect_id;
        udp_id_2_endpoint_list[connect_id] = recv_endpoint_;
        return connect_id;
    }
}

udp::endpoint CUdpServer::find_udp_endpoint_by_id(uint32 connect_id)
{
    udp::endpoint send_endpoint;
    auto f = udp_id_2_endpoint_list.find(connect_id);
    if (f != udp_id_2_endpoint_list.end())
    {
        send_endpoint = f->second;
    }
    
    return send_endpoint;
}

