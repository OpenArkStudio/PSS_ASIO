#include "TcpSSLServer.h"

#ifdef SSL_SUPPORT

CTcpSSLServer::CTcpSSLServer(CreateIoContextCallbackFunc callback,
    std::string server_ip, 
    io_port_type port,
    uint32 packet_parse_id, 
    uint32 max_recv_size, 
    std::string ssl_server_password,
    std::string ssl_server_pem_file,
    std::string ssl_server_dh_file,
    CIo_List_Manager* io_list_manager) :
context_(asio::ssl::context::sslv23),
packet_parse_id_(packet_parse_id),
max_recv_size_(max_recv_size),
ssl_server_password_(ssl_server_password),
ssl_server_pem_file_(ssl_server_pem_file),
ssl_server_dh_file_(ssl_server_dh_file),
io_list_manager_(io_list_manager)
{
    try
    {
        callback_ = callback;
        asio::io_context* iocontext = callback_();
        acceptor_ = std::make_shared<tcp::acceptor>(*iocontext, tcp::endpoint(asio::ip::address_v4::from_string(server_ip), port));

        context_.set_options(
            asio::ssl::context::default_workarounds
            | asio::ssl::context::no_sslv2
            | asio::ssl::context::single_dh_use);

        server_ip_ = server_ip;
        server_port_ = port;
        
        context_.set_password_callback(std::bind(&CTcpSSLServer::get_password, this));
        context_.use_certificate_chain_file(ssl_server_pem_file_);
        context_.use_private_key_file(ssl_server_pem_file_, asio::ssl::context::pem);
        context_.use_tmp_dh_file(ssl_server_dh_file_);

        //处理链接建立消息
        PSS_LOGGER_INFO("[CTcpSSLServer::do_accept]({0}:{1}) Begin Accept.",
            acceptor_->local_endpoint().address().to_string(),
            acceptor_->local_endpoint().port());

        do_accept();
    }
    catch (std::system_error const& ex)
    {
        PSS_LOGGER_DEBUG("[CTcpSSLServer::CTcpSSLServer]({0}:{1}) error={2}", server_ip, port, ex.what());
    }
}

void CTcpSSLServer::start()
{
    io_list_manager_->add_accept_net_io_event(server_ip_, server_port_, EM_CONNECT_IO_TYPE::CONNECT_IO_SSL, shared_from_this());
    do_accept();
}

void CTcpSSLServer::close() const
{
    acceptor_->close();
}

std::string CTcpSSLServer::get_password() const
{
    return ssl_server_password_;
}

void CTcpSSLServer::do_accept()
{
    acceptor_->async_accept(
        [this](const std::error_code& error, tcp::socket socket)
        {
            if (!error)
            {
                std::make_shared<CTcpSSLSession>(
                    asio::ssl::stream<tcp::socket>(
                        std::move(socket), context_), callback_())->open(packet_parse_id_, max_recv_size_);
            }
            else
            {
                PSS_LOGGER_DEBUG("[CTcpSSLServer::do_accept]listen error={0}", error.message());
                return;
            }

            do_accept();
        });
}

void CTcpSSLServer::send_accept_listen_fail(std::error_code ec) const
{
    //发送监听失败消息
    App_WorkThreadLogic::instance()->add_frame_events(LOGIC_LISTEN_SERVER_ERROR,
        0,
        acceptor_->local_endpoint().address().to_string(),
        acceptor_->local_endpoint().port(),
        EM_CONNECT_IO_TYPE::CONNECT_IO_SSL);

    //监听失败，查看错误信息
    PSS_LOGGER_INFO("[CTcpServer::do_accept]({0}{1})accept error:{2}",
        acceptor_->local_endpoint().address().to_string(),
        acceptor_->local_endpoint().port(),
        ec.message());

    io_list_manager_->del_accept_net_io_event(server_ip_, server_port_, EM_CONNECT_IO_TYPE::CONNECT_IO_SSL);
}

#endif
