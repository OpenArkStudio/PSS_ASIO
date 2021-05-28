#include "TcpServer.h"

CTcpServer::CTcpServer(asio::io_context& io_context, const std::string& server_ip, short port, uint32 packet_parse_id, uint32 max_buffer_size)
    : packet_parse_id_(packet_parse_id), max_recv_size_(max_buffer_size)
{
    try
    {
        acceptor_ = std::make_shared<tcp::acceptor>(io_context, tcp::endpoint(asio::ip::address_v4::from_string(server_ip), port));

        //处理链接建立消息
        PSS_LOGGER_INFO("[CTcpServer::do_accept]({0}:{1}) Begin Accept.",
            acceptor_->local_endpoint().address().to_string(),
            acceptor_->local_endpoint().port());

        do_accept();
    }
    catch (std::system_error const& ex)
    {
        PSS_LOGGER_INFO("[CTcpServer::do_accept]({0}:{1}) accept error {2}.", server_ip, port, ex.what());
    }
}

void CTcpServer::close() const
{
    if (nullptr != acceptor_)
    {
        acceptor_->close();
    }
}

void CTcpServer::do_accept()
{
    acceptor_->async_accept(
        [this](std::error_code ec, tcp::socket socket)
        {
            if (!ec)
            {
                std::make_shared<CTcpSession>(std::move(socket))->open(packet_parse_id_, max_recv_size_);
            }
            else
            {
                send_accept_listen_fail(ec);
            }

            do_accept();
        });
}

void CTcpServer::send_accept_listen_fail(std::error_code ec) const
{
    //发送监听失败消息
    App_WorkThreadLogic::instance()->add_frame_events(LOGIC_LISTEN_SERVER_ERROR,
        0,
        acceptor_->local_endpoint().address().to_string(),
        acceptor_->local_endpoint().port(),
        EM_CONNECT_IO_TYPE::CONNECT_IO_TCP);

    //监听失败，查看错误信息
    PSS_LOGGER_INFO("[CTcpServer::do_accept]({0}{1})accept error:{2}",
        acceptor_->local_endpoint().address().to_string(),
        acceptor_->local_endpoint().port(),
        ec.message());
}

