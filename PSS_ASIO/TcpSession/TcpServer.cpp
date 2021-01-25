#include "TcpServer.h"

CTcpServer::CTcpServer(asio::io_context& io_context, std::string server_ip, short port, uint32 packet_parse_id, uint32 max_buffer_size, uint32 max_send_size)
    : acceptor_(io_context, tcp::endpoint(asio::ip::address_v4::from_string(server_ip), port)), packet_parse_id_(packet_parse_id), max_recv_size_(max_buffer_size), max_send_size_(max_send_size)
{
    //处理链接建立消息
    std::cout << "[CTcpServer::do_accept](" << acceptor_.local_endpoint() << ") Begin Accept." << std::endl;

    do_accept();
}

void CTcpServer::close()
{
    acceptor_.close();
}

void CTcpServer::do_accept()
{
    acceptor_.async_accept(
        [this](std::error_code ec, tcp::socket socket)
        {
            if (!ec)
            {
                std::make_shared<CTcpSession>(std::move(socket))->open(packet_parse_id_, max_recv_size_, max_send_size_);
            }
            else
            {
                //监听失败，查看错误信息
                std::cout << ec.message() << std::endl;
            }

            do_accept();
        });
}

