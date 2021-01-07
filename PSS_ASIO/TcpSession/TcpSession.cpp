#include "TcpSession.h"

CTcpSession::CTcpSession(tcp::socket socket)
    : socket_(std::move(socket))
{
}

void CTcpSession::open(uint32 connect_id, uint32 packet_parse_id)
{
    connect_id_ = connect_id;

    packet_parse_interface_ = App_PacketParseLoader::instance()->GetPacketParseInfo(packet_parse_id);

    session_recv_buffer_.Init(1024);

    //处理链接建立消息
    _ClientIPInfo remote_ip;
    _ClientIPInfo local_ip;
    remote_ip.m_strClientIP = socket_.remote_endpoint().address().to_string();
    remote_ip.m_u2Port = socket_.remote_endpoint().port();
    local_ip.m_strClientIP = socket_.local_endpoint().address().to_string();
    local_ip.m_u2Port = socket_.local_endpoint().port();
    packet_parse_interface_->Connect(connect_id_, remote_ip, local_ip);

    do_read();
}

void CTcpSession::Close()
{
    socket_.close();

    //断开连接
    packet_parse_interface_->DisConnect(connect_id_);
}

void CTcpSession::do_read()
{
    //接收数据
    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(session_recv_buffer_.get_curr_write_ptr(), session_recv_buffer_.get_buffer_size()),
        [this, self](std::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                session_recv_buffer_.set_write_data(length);

                //处理数据拆包
                vector<CMessage_Packet> message_list;
                bool ret = packet_parse_interface_->Parse_Packet_From_Recv_Buffer(connect_id_, &session_recv_buffer_, message_list, EM_CONNECT_IO_TYPE::CONNECT_IO_TCP);
                if (!ret)
                {
                    //链接断开(解析包不正确)
                    App_tms::instance()->AddMessage(1, [self]() {
                        self->Close();
                        });
                }

                //添加到数据队列处理
                App_tms::instance()->AddMessage(1, [self, message_list](){
                    CMessage_Packet send_packet;
                    for (auto packet : message_list)
                    {
                        send_packet.body_.append(packet.head_.c_str(), packet.head_.size());
                        send_packet.body_.append(packet.body_.c_str(), packet.body_.size());
                    }

                    self->do_write(send_packet);
                    });

                //继续读数据
                self->do_read();
            }
            else
            {
                //链接断开
                App_tms::instance()->AddMessage(1, [self]() {
                    self->Close();
                    });
            }
        });
}

void CTcpSession::do_write(CMessage_Packet send_packet)
{
    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(send_packet.body_.c_str(), send_packet.body_.size());
    send_buffer->buffer_length_ = send_packet.body_.size();

    //异步发送
    asio::async_write(socket_, asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_),
        [send_buffer](std::error_code ec, std::size_t)
        {
            if (ec)
            {
                //暂时不处理
                std::cout << "[CTcpSession::do_write](" << ec.value() << ") mseeage(" << ec.message() << ")" << std::endl;
            }
        });
}

