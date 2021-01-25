#pragma once

//读取参数表定义
//add freeeyes

#include <string>
#include <vector>

//定义参数类型表
class CConfigWorkThread
{
public:
    int work_thread_count_ = 1;
};

class CConfigPacketParseInfo
{
public:
    int packet_parse_id_ = 0;
    std::string packet_parse_path_;
    std::string packet_parse_file_name_;
};

using config_packet_list = std::vector<CConfigPacketParseInfo>;

class CConfigLogicInfo
{
public:
    std::string logic_path_;
    std::string logic_file_name_;
    std::string logic_param_;
};

using config_logic_list = std::vector<CConfigLogicInfo>;

class CConfigNetIO
{
public:
    std::string ip_;
    unsigned short port_ = 0;
    unsigned int packet_parse_id_ = 0;
    unsigned int recv_buff_size_ = 1024;
    unsigned int send_buff_size_ = 1024;
};

using config_tcp_list = std::vector<CConfigNetIO>;
using config_udp_list = std::vector<CConfigNetIO>;

class CConfigConsole
{
public:
    bool file_output_ = false;
    int file_count_ = 3;
    int max_file_size_ = 1048576;
    std::string file_name_;
    std::string output_level_ = "debug";
};
