#pragma once

#include <iostream>
#include <fstream>
#include "json/json.hpp"
#include "serverconfigtype.h"

const std::string config_file_name = "server_config.json";

using json = nlohmann::json;

class CServerConfig
{
public:
    CServerConfig() = default;

    bool read_server_config_file(std::string file_name = config_file_name);

    config_packet_list& get_config_packet_list();
    config_logic_list& get_config_logic_list();
    config_tcp_list& get_config_tcp_list();
    config_udp_list& get_config_udp_list();
    config_tty_list& get_config_tty_list();
    CConfigConsole& get_config_console();
    CConfigWorkThread& get_config_workthread();

private:
    config_packet_list config_packet_list_;
    config_logic_list config_logic_list_;
    config_tcp_list config_tcp_list_;
    config_udp_list config_udp_list_;
    config_tty_list config_tty_list_;
    CConfigConsole config_output_;
    CConfigWorkThread config_work_thread_;
};
