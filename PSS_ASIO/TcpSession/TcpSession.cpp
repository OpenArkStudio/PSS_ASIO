#include "TcpSession.h"

CTcpSession::CTcpSession(tcp::socket socket)
    : socket_(std::move(socket))
{
}

void CTcpSession::open(uint32 connect_id, uint32 packet_parse_id, uint32 buffer_size)
{
    connect_id_ = connect_id;

    packet_parse_interface_ = App_PacketParseLoader::instance()->GetPacketParseInfo(packet_parse_id);

    session_recv_buffer_.Init(buffer_size);
    session_send_buffer_.Init(buffer_size);

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

    //输出接收发送字节数
    PSS_LOGGER_DEBUG("[CTcpSession::Close]recv:{0}, send:{1}", recv_data_size_, send_data_size_);

    //断开连接
    packet_parse_interface_->DisConnect(connect_id_);
}

void CTcpSession::do_read()
{
    //接收数据
    auto self(shared_from_this());

    //如果缓冲已满，断开连接，不再接受数据。
    if (session_recv_buffer_.get_buffer_size() == 0)
    {
        //链接断开(缓冲撑满了)
        App_tms::instance()->AddMessage(1, [self]() {
            self->Close();
            });
    }

    socket_.async_read_some(asio::buffer(session_recv_buffer_.get_curr_write_ptr(), session_recv_buffer_.get_buffer_size()),
        [this, self](std::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                recv_data_size_ += length;
                session_recv_buffer_.set_write_data(length);
                //PSS_LOGGER_DEBUG("[CTcpSession::do_write]recv length={}.", length);
                
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
                    //PSS_LOGGER_DEBUG("[CTcpSession::AddMessage]count={}.", message_list.size());
                    for (auto packet : message_list)
                    {
                        self->set_write_buffer(packet.head_.c_str(), packet.head_.size());
                        self->set_write_buffer(packet.body_.c_str(), packet.body_.size());
                    }

                    self->do_write();
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

void CTcpSession::do_write()
{
    //组装发送数据
    auto send_buffer = make_shared<CSendBuffer>();
    send_buffer->data_.append(session_send_buffer_.read(), session_send_buffer_.get_write_size());
    send_buffer->buffer_length_ = session_send_buffer_.get_write_size();

    //PSS_LOGGER_DEBUG("[CTcpSession::do_write]send_buffer->buffer_length_={}.", send_buffer->buffer_length_);
    clear_write_buffer();

    //异步发送
    auto self(shared_from_this());
    asio::async_write(socket_, asio::buffer(send_buffer->data_.c_str(), send_buffer->buffer_length_),
        [self, send_buffer](std::error_code ec, std::size_t length)
        {
            if (ec)
            {
                //暂时不处理
                std::cout << "[CTcpSession::do_write](" << ec.value() << ") mseeage(" << ec.message() << ")" << std::endl;
            }
            else
            {
                self->add_send_finish_size(length);
            }
        });
}

void CTcpSession::set_write_buffer(const char* data, size_t length)
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

void CTcpSession::clear_write_buffer()
{
    session_send_buffer_.move(session_send_buffer_.get_write_size());
}

void CTcpSession::add_send_finish_size(size_t send_length)
{
    send_data_size_ += send_length;
}

