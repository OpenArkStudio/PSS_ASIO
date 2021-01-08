// PSS_ASIO_CLIENT.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <string>
#include "asio.hpp"
using asio::ip::tcp;


void test_connect_server(std::string strIP, unsigned short port)
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
    unsigned short client_command_id = 1;
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
    test_connect_server("127.0.0.1", 8888);

    getchar();
}