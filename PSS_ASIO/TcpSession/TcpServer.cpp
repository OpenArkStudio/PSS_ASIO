#include "TcpServer.h"

CTcpServer::CTcpServer(const CreateIoContextCallbackFunc& callback, const std::string& server_ip, io_port_type port, uint32 packet_parse_id, uint32 max_buffer_size, CIo_List_Manager* io_list_manager)
    : packet_parse_id_(packet_parse_id), max_recv_size_(max_buffer_size),server_ip_(server_ip),server_port_(port), io_list_manager_(io_list_manager)
{
    try
    {
        callback_ = callback;
        asio::io_context* iocontext = callback_();
        acceptor_ = std::make_shared<tcp::acceptor>(*iocontext, tcp::endpoint(asio::ip::address_v4::from_string(server_ip), port));

        //处理链接建立消息
        PSS_LOGGER_INFO("[CTcpServer::do_accept]({0}:{1}) Begin Accept.",
            acceptor_->local_endpoint().address().to_string(),
            acceptor_->local_endpoint().port());
    }
    catch (std::system_error const& ex)
    {
        PSS_LOGGER_WARN("[CTcpServer::do_accept]({0}:{1}) accept error {2}.", server_ip, port, ex.what());
    }
}

void CTcpServer::start()
{
    io_list_manager_->add_accept_net_io_event(server_ip_, server_port_, EM_CONNECT_IO_TYPE::CONNECT_IO_TCP, shared_from_this());
    do_accept();
}

void CTcpServer::close()
{
    PSS_LOGGER_INFO("[CTcpServer::close]stop tcp accept bgein[{0}:{1}]", server_ip_, server_port_);
    acceptor_->close();
}

void CTcpServer::do_accept()
{   
    acceptor_->async_accept(
        [this](std::error_code ec, tcp::socket socket)
        {
            try 
            {
                if (!ec)
                {
                    std::make_shared<CTcpSession>(std::move(socket), callback_())->open(packet_parse_id_, max_recv_size_);
                }
                else
                {
                    send_accept_listen_fail(ec);
                    return;
                }

                do_accept();
            } 
            catch (std::system_error const& ex) 
            {
                PSS_LOGGER_WARN("[CTcpServer::do_accept]close tcp server[{}:{}], error={}",server_ip_,server_port_, ex.what());
            }
        });
}

void CTcpServer::send_accept_listen_fail(std::error_code ec)
{
    //发送监听失败消息
    auto server_ip = server_ip_;
    auto server_port = server_port_;
    App_WorkThreadLogic::instance()->add_frame_events(LOGIC_LISTEN_SERVER_ERROR,
        0,
        server_ip,
        server_port,
        EM_CONNECT_IO_TYPE::CONNECT_IO_TCP);

    //监听失败，查看错误信息
    PSS_LOGGER_INFO("[CTcpServer::do_accept]({0}:{1})accept error:{2}",
        server_ip_,
        server_port_,
        ec.message());

    io_list_manager_->del_accept_net_io_event(server_ip_, server_port_, EM_CONNECT_IO_TYPE::CONNECT_IO_TCP);
}

