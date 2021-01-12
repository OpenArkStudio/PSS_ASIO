// PSS_ASIO.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "TcpServer.h"
#include "UdpServer.h"
#include "TtyServer.h"

#if PSS_PLATFORM == PLATFORM_WIN
#include <tchar.h>
#endif

void add_serial_port(asio::io_context& io_context)
{
    int char_size = 8;
    std::error_code ec;
    auto serial_port = std::make_shared<asio::serial_port>(io_context);

    serial_port->set_option(asio::serial_port::baud_rate(9600), ec);
    serial_port->set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none), ec);
    serial_port->set_option(asio::serial_port::parity(asio::serial_port::parity::none), ec);
    serial_port->set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one), ec);
    serial_port->set_option(asio::serial_port::character_size(char_size), ec);


}

int main()
{
    //指定当前目录，防止访问文件失败
    TCHAR szFileName[MAX_PATH] = { 0 };
    GetModuleFileName(0, szFileName, MAX_PATH);
    LPTSTR pszEnd = _tcsrchr(szFileName, TEXT('\\'));

    if (pszEnd != 0)
    {
        pszEnd++;
        *pszEnd = 0;
    }

    //初始化输出
    Init_Console_Output(0,
        1,
        1024000,
        "./serverlog",
        "debug");

    //初始化PacketParse插件
    App_PacketParseLoader::instance()->Init(100);

    if (false == App_PacketParseLoader::instance()->LoadPacketInfo(1, "./", "PacketParse_Inferface.dll"))
    {
        PSS_LOGGER_DEBUG("[App_PacketParseLoader] load error.");
    }

    int socket_serevr_port = 8888;
    asio::io_context io_context;

    App_tms::instance()->CreateLogic(1);

    //测试Tcp监听
    CTcpServer s1(io_context, "127.0.0.1", socket_serevr_port, 1, 102400);

    //测试UDP监听
    CUdpServer s2(io_context, "127.0.0.1", socket_serevr_port, 1, 1024);
    io_context.run();

}
