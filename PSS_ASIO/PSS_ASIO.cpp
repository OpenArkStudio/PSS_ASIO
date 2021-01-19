// PSS_ASIO.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "ServerService.h"

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
    App_ServerService::instance()->init_servce();

    return 0;
}
