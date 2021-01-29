// PSS_ASIO_CLIENT.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <string>

#include "TcpSession.h"
#include "UdpSession.h"

//异步客户端(udp)
void udp_test_connect_asynchronous_server(std::string strIP, unsigned short port)
{
    asio::io_context io_context;

    auto c = make_shared<CUdpSession>(io_context);

    c->start(1, 2400, "127.0.0.1", 8888);

    //发送数据
    char send_buffer[240] = { '\0' };
    int nPos = 0;

    unsigned short client_version = 1;
    unsigned short client_command_id = 0x2101;
    unsigned int client_packet_length = 200;

    std::memcpy(&send_buffer[nPos], &client_version, sizeof(short));
    nPos += sizeof(short);
    std::memcpy(&send_buffer[nPos], &client_command_id, sizeof(short));
    nPos += sizeof(short);
    std::memcpy(&send_buffer[nPos], &client_packet_length, sizeof(int));
    nPos += sizeof(int);
    nPos += 32;
    nPos += 200;

    c->set_write_buffer(send_buffer, 240);
    c->do_write();

    io_context.run();
}

//异步客户端(tcp)
void test_connect_asynchronous_server(std::string strIP, unsigned short port)
{
    asio::io_context io_context;

    auto c = make_shared<CTcpSession>(io_context);

    c->start(1, 2400, "127.0.0.1", 8888);

    //发送数据
    char send_buffer[2400] = { '\0' };
    int nPos = 0;

    unsigned short client_version = 1;
    unsigned short client_command_id = 0x2101;
    unsigned int client_packet_length = 200;


    for (int i = 0; i < 10; i++)
    {
        std::memcpy(&send_buffer[nPos], &client_version, sizeof(short));
        nPos += sizeof(short);
        std::memcpy(&send_buffer[nPos], &client_command_id, sizeof(short));
        nPos += sizeof(short);
        std::memcpy(&send_buffer[nPos], &client_packet_length, sizeof(int));
        nPos += sizeof(int);
        nPos += 32;
        nPos += 200;
    }

    c->set_write_buffer(send_buffer, 2400);
    c->do_write();

    io_context.run();

}

//同步客户端(tcp)
void test_connect_synchronize_server(std::string strIP, unsigned short port)
{
    asio::io_context io_context;

    tcp::socket s(io_context);
    tcp::resolver resolver(io_context);
    tcp::endpoint end_point(asio::ip::address::from_string(strIP.c_str()), port);
    //asio::connect(s, end_point);
    asio::error_code connect_error;
    s.connect(end_point, connect_error);

    if (connect_error)
    {
        //链接失败
        std::cout << "[test_connect_server]connect error(" << connect_error.message() << std::endl;
        return;
    }

    std::cout << "[test_connect_server]connect OK" << std::endl;

    //发送数据
    char send_buffer[2400] = { '\0' };
    int nPos = 0;

    unsigned short client_version = 1;
    unsigned short client_command_id = 0x2101;
    unsigned int client_packet_length = 200;


    for (int i = 0; i < 10; i++)
    {
        std::memcpy(&send_buffer[nPos], &client_version, sizeof(short));
        nPos += sizeof(short);
        std::memcpy(&send_buffer[nPos], &client_command_id, sizeof(short));
        nPos += sizeof(short);
        std::memcpy(&send_buffer[nPos], &client_packet_length, sizeof(int));
        nPos += sizeof(int);
        nPos += 32;
        nPos += 200;
    }

    asio::write(s, asio::buffer(send_buffer, 2400));

    //接收数据
    char recv_buffer[2400] = { '\0' };
    asio::error_code error;

    size_t recv_all_size = 0;
    while (true)
    {
        size_t max_buffer = 2400;
        size_t reply_length = asio::read(s, asio::buffer(recv_buffer, 2400));
        recv_all_size += reply_length;
        if (recv_all_size == 2400)
        {
            break;
        }
    }

    s.close();
}

int main()
{
    //初始化输出
    Init_Console_Output(0,
        1,
        1024000,
        "./serverlog",
        "debug");

    App_tms::instance()->CreateLogic(1);
    //test_connect_synchronize_server("127.0.0.1", 8888);

    //test_connect_asynchronous_server("127.0.0.1", 8888);

    //udp_test_connect_asynchronous_server("127.0.0.1", 8888);

    getchar();
}