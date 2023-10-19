#include "TcpServer.h"

CTcpServer::CTcpServer(const CreateIoContextCallbackFunc& callback, const CConfigNetIO& config_io, CIo_List_Manager* io_list_manager)
    : io_list_manager_(io_list_manager)
{
    try
    {
        packet_parse_id_ = config_io.packet_parse_id_;
        max_recv_size_ = config_io.recv_buff_size_;
        server_ip_ = config_io.ip_;
        server_port_ = config_io.port_;

        callback_ = callback;
        asio::io_context* iocontext = callback_();
        acceptor_ = std::make_shared<tcp::acceptor>(*iocontext, tcp::endpoint(asio::ip::address_v4::from_string(server_ip_), server_port_));

        //处理链接建立消息
        PSS_LOGGER_INFO("[CTcpServer::do_accept]({0}:{1}) Begin Accept.",
            acceptor_->local_endpoint().address().to_string(),
            acceptor_->local_endpoint().port());
    }
    catch (std::system_error const& ex)
    {
        PSS_LOGGER_WARN("[CTcpServer::do_accept]({0}:{1}) accept error {2}.", server_ip_, server_port_, ex.what());
    }
}

void CTcpServer::start()
{
    io_list_manager_->add_accept_net_io_event(server_ip_, server_port_, EM_CONNECT_IO_TYPE::CONNECT_IO_TCP, shared_from_this());
    PSS_LOGGER_INFO("[CTcpServer::start]tcp accept start[{0}:{1}]", server_ip_, server_port_);
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
    App_WorkThreadLogic::instance()->add_frame_events(LOGIC_LISTEN_SERVER_ERROR,
        0,
        server_ip_,
        server_port_,
        EM_CONNECT_IO_TYPE::CONNECT_IO_TCP);

    //监听失败，查看错误信息
    PSS_LOGGER_INFO("[CTcpServer::do_accept]({0}:{1})accept error:{2}",
        server_ip_,
        server_port_,
        ec.message());

    io_list_manager_->del_accept_net_io_event(server_ip_, server_port_, EM_CONNECT_IO_TYPE::CONNECT_IO_TCP);
}

