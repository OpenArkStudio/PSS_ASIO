// PSS_ASIO_CLIENT.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <string>

#include "TcpSession.h"
#include "UdpSession.h"
#include "kcp_client.h"

void kcp_test_connect_server(int kcp_id, kcpObj& send)
{
    ikcpcb* kcp = ikcp_create(kcp_id, (void*)&send);//创建kcp对象把send传给kcp的user变量
    kcp->output = udpOutPut;//设置kcp对象的回调函数
    ikcp_nodelay(kcp, 0, 10, 0, 0);//(kcp1, 0, 10, 0, 0); 1, 10, 2, 1
    ikcp_wndsize(kcp, 128, 128);

    send.pkcp = kcp;

    //发送数据
    char send_buffer[48] = { '\0' };
    int nPos = 0;

    unsigned short client_version = 1;
    unsigned short client_command_id = 0x2101;
    unsigned int client_packet_length = 8;

    std::memcpy(&send_buffer[nPos], &client_version, sizeof(short));
    nPos += sizeof(short);
    std::memcpy(&send_buffer[nPos], &client_command_id, sizeof(short));
    nPos += sizeof(short);
    std::memcpy(&send_buffer[nPos], &client_packet_length, sizeof(int));
    nPos += sizeof(int);
    nPos += 32;
    nPos += 8;
    int	ret = ikcp_send(send.pkcp, send_buffer, 48);
    printf("ikcp_send发送连接请求: len=48 ret = %d\n", ret);//发送成功的

    loop(&send);//循环处理

    ikcp_release(send.pkcp);
}


void from_server_get_kcp_id(std::string strIP, unsigned short port)
{
    kcpObj send;
#if defined(WIN32)
    int recvlen = sizeof(struct sockaddr_in);
#else
    unsigned int recvlen = sizeof(struct sockaddr_in);
#endif
    int kcp_id = 0;

    send.ipstr = strIP;
    send.port = port;

    init(&send);//初始化send,主要是设置与服务器通信的套接字对象

    std::string kcp_key_id_create = "kcp key id create";
    sendto(send.sockfd, kcp_key_id_create.c_str(), kcp_key_id_create.length(), 0, (struct sockaddr*)&send.addr, sizeof(struct sockaddr_in));

    //接收对应的key
    char buf[512] = { '\0' };
    auto recv_size = recvfrom(send.sockfd, buf, 512, 0, (struct sockaddr*)&send.addr, &recvlen);
    if (recv_size > 0)//检测是否有UDP数据包
    {
        if (recv_size == 4)
        {
            std::memcpy(&kcp_id, &buf[0], (int)sizeof(int));
        }
    }

    kcp_test_connect_server(kcp_id, send);
}

//异步客户端(udp)
void udp_test_connect_asynchronous_server(std::string strIP, unsigned short port, asio::io_context& io_context)
{
    auto c = make_shared<CUdpSession>(io_context);

    c->start(1, 2400, strIP, port);

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
}

//异步客户端(tcp)
void tcp_test_connect_asynchronous_server(std::string strIP, unsigned short port, asio::io_context& io_context)
{
    auto c = make_shared<CTcpSession>(io_context);

    c->start(1, 2400, strIP, port);

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
}

//同步客户端(tcp)
void tcp_test_connect_synchronize_server(std::string strIP, unsigned short port, unsigned short remote_port, uint16 command_id, uint16 packt_count, asio::io_context& io_context)
{
    tcp::socket s(io_context);
    tcp::resolver resolver(io_context);
    tcp::endpoint end_point(asio::ip::address::from_string(strIP.c_str()), port);

    asio::error_code connect_error;
    asio::ip::tcp::endpoint localEndpoint(asio::ip::address::from_string(strIP.c_str()), remote_port);
    
    s.open(asio::ip::tcp::v4(), connect_error);
    if (connect_error)
    {
        std::cout << "[tcp_test_connect_synchronize_server]open error(" << connect_error.message() << std::endl;
        return;
    }

    s.bind(localEndpoint, connect_error);
    if (connect_error)
    {
        //链接失败
        std::cout << "[tcp_test_connect_synchronize_server]bind error(" << connect_error.message() << std::endl;
        return;
    }

    s.set_option(asio::ip::tcp::socket::reuse_address(true));
    s.connect(end_point, connect_error);

    if (connect_error)
    {
        //链接失败
        std::cout << "[tcp_test_connect_synchronize_server]connect error(" << connect_error.message() << std::endl;
        return;
    }

    std::cout << "[tcp_test_connect_synchronize_server]("<< command_id << ")connect OK" << std::endl;

    //发送数据
    char* send_buffer = new char [240 * packt_count];
    int nPos = 0;

    unsigned short client_version = 1;
    unsigned short client_command_id = command_id;
    unsigned int client_packet_length = 200;

    for (int i = 0; i < packt_count; i++)
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

    std::size_t send_size = asio::write(s, asio::buffer(send_buffer, 240 * packt_count));
	
	std::cout << "[tcp_test_connect_synchronize_server]("<< command_id << ")send （" << send_size << ") OK" << std::endl;

    //接收数据
    char* recv_buffer = new char[240 * packt_count];
    asio::error_code error;

    size_t recv_all_size = 0;
    while (true)
    {
        size_t reply_length = asio::read(s, asio::buffer(recv_buffer, 240 * packt_count));
        recv_all_size += reply_length;
        if (recv_all_size == 240 * packt_count)
        {
            break;
        }
    }

	std::cout << "[tcp_test_connect_synchronize_server]("<< command_id << ")send OK" << std::endl;	
    delete[] send_buffer;
    delete[] recv_buffer;

    s.close();
}

//同步客户端(udp)
void udp_test_connect_synchronize_server(std::string strIP, unsigned short port, unsigned short remote_port, uint16 command_id, asio::io_context& io_context)
{
    udp::endpoint end_point(asio::ip::address::from_string(strIP.c_str()), port);

    udp::socket sock(io_context, udp::endpoint(asio::ip::address::from_string("127.0.0.1"), remote_port));

    std::cout << "[udp_test_connect_synchronize_server]connect OK" << std::endl;

    //发送数据
    char send_buffer[240] = { '\0' };
    int nPos = 0;

    unsigned short client_version = 1;
    unsigned short client_command_id = command_id;
    unsigned int client_packet_length = 200;

    std::memcpy(&send_buffer[nPos], &client_version, sizeof(short));
    nPos += sizeof(short);
    std::memcpy(&send_buffer[nPos], &client_command_id, sizeof(short));
    nPos += sizeof(short);
    std::memcpy(&send_buffer[nPos], &client_packet_length, sizeof(int));
    nPos += sizeof(int);
    nPos += 32;
    nPos += 200;

    sock.send_to(asio::buffer(send_buffer, 240), end_point);

    //接收数据
    char recv_buffer[240] = { '\0' };
    asio::error_code error;

    udp::endpoint recv_ep;
    size_t recv_size = sock.receive_from(asio::buffer(recv_buffer, 240), recv_ep);
    if (recv_size == 240)
    {
        std::cout << "[udp_test_connect_synchronize_server]udp test ok." << std::endl;
    }
    else
    {
        std::cout << "[udp_test_connect_synchronize_server]udp test fail." << std::endl;
    }

    std::cout << "[udp_test_connect_synchronize_server]udp test ok." << std::endl;
    sock.close();
}

int main()
{
    //初始化输出
    Init_Console_Output(0,
        1,
        102400,
        "./testclientlog",
        "debug");

    asio::io_context io_context;

    std::thread tt = std::thread([&io_context]()
        {
            io_context.run();
        });

    tcp_test_connect_synchronize_server("127.0.0.1", 10002, 10010, 0x2101, 1, io_context);
    tcp_test_connect_synchronize_server("127.0.0.1", 10002, 10011, 0x2102, 1, io_context);

    udp_test_connect_synchronize_server("127.0.0.1", 10005, 10012, 0x2101, io_context);
    udp_test_connect_synchronize_server("127.0.0.1", 10005, 10012, 0x2102, io_context);
    from_server_get_kcp_id("127.0.0.1", 10100);

    io_context.stop();
    tt.join();

    std::cout << "test finish." << std::endl;

    return 0;
}