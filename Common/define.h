﻿#pragma once

#include <math.h>
#include <type_traits>
#include <fstream>
#include <chrono>
#include <iostream>

#include "VersionConfig.h"
#include "consoleoutput.hpp"

using namespace std;

//自动判定操作系统
#define PLATFORM_WIN     0
#define PLATFORM_UNIX    1
#define PLATFORM_APPLE   2

#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN64__) || defined(WIN64) || defined(_WIN64)
#  define PSS_PLATFORM PLATFORM_WIN
#elif defined(__APPLE_CC__)
#  define PSS_PLATFORM PLATFORM_APPLE
#else
#  define PSS_PLATFORM PLATFORM_UNIX
#endif

//是否开启字节序检测
#define BYTE_SORT_SWITCH_OFF 0
#define BYTE_SORT_SWITCH_ON 1
#define BYTE_SORT_SWITCH_STATE BYTE_SORT_SWITCH_OFF

//基础类型定义
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using float32 = float;
using float64 = double;
using io_port_type = uint16_t;

const size_t TCP_BUFFER_SIZE = 10 * 1024 * 1024;

//IO桥接类型
enum class ENUM_IO_BRIDGE_TYPE
{
    IO_BRIDGE_BATH = 0,    //双向桥接
    IO_BRIDGE_FROM,        //从from到to
    IO_BRIDGE_TO,          //从to到from
    IO_BRIDGE_NONE,        //没有桥接
};

enum class ENUM_WHILE_STATE
{
    WHILE_STATE_CONTINUE = 0,
    WHILE_STATE_BREAK,
};

//连接的性质类型
enum class EM_CONNECT_IO_TYPE
{
    CONNECT_IO_TCP = 0,      //IO是TCP
    CONNECT_IO_UDP,          //IO是UDP
    CONNECT_IO_KCP,          //IO是UDP的KCP
    CONNECT_IO_TTY,          //IO是TTY
    CONNECT_IO_SSL,          //IO是SSL
    CONNECT_IO_SERVER_TCP,   //IO是服务期间连接TCP
    CONNECT_IO_SERVER_UDP,   //IO是服务期间连接UDP
    CONNECT_IO_FRAME,        //来自插件间的回调
    COMMAND_UPDATE,          //来自插件更新  
    WORKTHREAD_CLOSE         //关闭当前工作线程
};

//连接类型(目前只包括UDP的点对点和广播)
enum class EM_NET_TYPE
{
    NET_TYPE_POINT_TO_POINT = 0,  //正常点对点网络 
    NET_TYPE_BROADCAST,           //UDP广播网络
};

//IO链接状态
enum class EM_SESSION_STATE
{
    SESSION_IO_LOGIC = 0,     //IO消息工作线程处理状态
    SESSION_IO_BRIDGE,        //IO消息桥接状态
};

//监听服务的IO接口信息
class CConfigNetIO
{
public:
    std::string ip_ = "";
    io_port_type port_ = 0;
    EM_CONNECT_IO_TYPE protocol_type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_TCP;
    unsigned int packet_parse_id_ = 0;
    unsigned int recv_buff_size_ = 1024;
    unsigned int send_buff_size_ = 1024;
    std::string ssl_server_password_;
    std::string ssl_server_pem_file_;
    std::string ssl_dh_pem_file_;
    EM_NET_TYPE em_net_type_ = EM_NET_TYPE::NET_TYPE_POINT_TO_POINT;
};

//客户端IP信息
class _ClientIPInfo
{
public:
    string m_strClientIP = "unset ip";      //客户端的IP地址
    uint16 m_u2Port = 0;                    //客户端的端口

    bool operator == (const _ClientIPInfo& other) const
    {
        return m_strClientIP == other.m_strClientIP && m_u2Port == other.m_u2Port;
    }

    bool operator < (const _ClientIPInfo& other) const 
    {
        if (this->m_strClientIP < other.m_strClientIP)return true;
        if (this->m_strClientIP > other.m_strClientIP)return false;
        if (this->m_u2Port < other.m_u2Port)return true;
        return false;
    }

    bool operator > (const _ClientIPInfo& other) const 
    {
        if (this->m_strClientIP > other.m_strClientIP)return true;
        if (this->m_strClientIP < other.m_strClientIP)return false;
        if (this->m_u2Port > other.m_u2Port)return true;
        return false;
    }

    // 赋值运算符重载函数
    _ClientIPInfo& operator = (const _ClientIPInfo& other)
    {
        // 避免自赋值
        if (this != &other)
        {
            m_strClientIP = other.m_strClientIP;
            m_u2Port = other.m_u2Port;
        }
        
        return *this;
    }
        
};

class CMessage_Source
{
public:
    uint16 work_thread_id_ = 0;     //当前工作线程ID
    uint32 connect_id_ = 0;         //当前链接ID
    uint32 connect_mark_id_ = 0;    //当前标记ID，用于服务器间链接的ID
    EM_CONNECT_IO_TYPE type_ = EM_CONNECT_IO_TYPE::CONNECT_IO_SERVER_TCP;
    _ClientIPInfo local_ip_;
    _ClientIPInfo remote_ip_;
};

//逻辑数据包结构
class CMessage_Packet
{
public:
    std::vector<string> param_arg_; //其他的透传参数    
    string buffer_;                 //数据内容
    uint16 command_id_;             //消息ID
};

//插件内调用延迟消息数据体
class CFrame_Message_Delay
{
public:
    std::chrono::seconds delay_seconds_ = std::chrono::seconds(0); //延迟的秒数
    uint64 timer_id_ = 0;  //定时器的id(这个必须唯一，否则会添加定时器失败)
};

//服务器间链接结构
class CConnect_IO_Info
{
public:
    uint32 server_id = 0;            //服务器ID
    std::string server_ip;           //远程服务器的IP
    io_port_type server_port = 0;    //远程服务器的端口
    std::string client_ip;           //发起端的IP(默认可以设置)
    io_port_type client_port = 0;    //发起端绑定的IP(默认可以不设置)
    uint32 packet_parse_id = 1;      //解码的PacketParse插件ID
    uint32 recv_size = 1024;         //接收数据最大缓冲
    uint32 send_size = 1024;         //发送数据最大缓冲
    std::string client_pem_file;     //对应客户端的pem文件
    bool is_need_reconnect = false;  //是否需要断线重连
};

//定义输出屏幕函数接口
inline void log_screen() {}

template<typename First, typename ...Rest>
void log_screen(First&& first, Rest && ...rest)
{
    std::cout << std::forward<First>(first);
    log_screen(std::forward<Rest>(rest)...);
}

//定义操作宏
#define PSS_LOGGER_DEBUG(...) SPDLOG_LOGGER_DEBUG(spdlog::default_logger(), __VA_ARGS__)
#define PSS_LOGGER_INFO(...) SPDLOG_LOGGER_INFO(spdlog::default_logger(), __VA_ARGS__)
#define PSS_LOGGER_WARN(...) SPDLOG_LOGGER_WARN(spdlog::default_logger(), __VA_ARGS__)
#define PSS_LOGGER_ERROR(...) SPDLOG_LOGGER_ERROR(spdlog::default_logger(), __VA_ARGS__)

//链接消息命令
const uint16 LOGIC_COMMAND_CONNECT = 0x0001;     //链接建立事件
const uint16 LOGIC_COMMAND_DISCONNECT = 0x0002;   //链接断开事件
const uint16 LOGIC_CONNECT_SERVER_ERROR = 0x0003;  //链接服务器不成功事件
const uint16 LOGIC_LISTEN_SERVER_ERROR = 0x0004;  //创建监听事件
const uint16 LOGIC_THREAD_DEAD_LOCK = 0x0005;  //创建工作线程死锁消息
const uint16 LOGIC_THREAD_WRITE_IO_ERROR = 0x0006;  //发送数据失败
const uint16 LOGIC_IOTOIO_CONNECT_NO_EXIST = 0x0007;  //透传数据的时候对端不存在
const uint16 LOGIC_IOTOIO_DATA_ERROR = 0x0008;    //桥接IO失败的IO数据
const uint16 LOGIC_MAX_FRAME_COMMAND = 0x0010;   //内部事件ID上限 

using task_function = std::function<void()>;

//暂不使用的参数
template <typename T>
void PSS_UNUSED_ARG(T&&)
{ }

inline void Init_Console_Output(bool blTurnOn, int nFileCount, int nLogFileMaxSize, string strConsoleName, string strLevel)
{
    Console_Output_Info obj_Console_Output_Info;
    obj_Console_Output_Info.m_blTunOn = blTurnOn;

    obj_Console_Output_Info.m_nFileCount = nFileCount;
    obj_Console_Output_Info.m_nLogFileMaxSize = nLogFileMaxSize;
    obj_Console_Output_Info.m_strConsoleName = strConsoleName;
    obj_Console_Output_Info.m_strLevel = strLevel;

    app_ConsoleOutput::instance()->Init(obj_Console_Output_Info);
}

inline std::chrono::seconds get_time_delay(std::string date)
{
    std::chrono::seconds delete_seconds;
    std::tm tm_;
    int year, month, day, hour, minute, second;// 定义时间的各个int临时变量。
#if PSS_PLATFORM != PLATFORM_WIN
    sscanf(date.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
#else
    sscanf_s(date.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
#endif

    tm_.tm_year = year - 1900;
    tm_.tm_mon = month - 1;
    tm_.tm_mday = day;
    tm_.tm_hour = hour;
    tm_.tm_min = minute;
    tm_.tm_sec = second;
    tm_.tm_isdst = 0;                          // 非夏令时。

    auto tp_tag = std::chrono::system_clock::from_time_t(mktime(&tm_));
    auto tp_now = std::chrono::system_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::seconds>(tp_tag - tp_now);

    //std::cout << "seconds=" << duration.count() << std::endl;
    delete_seconds = std::chrono::seconds(duration.count());
    return delete_seconds;
}

inline vector<std::string> string_split(const string& srcStr, const string& delim)
{
    size_t nPos = 0;
    vector<string> vec;
    string strtmp = srcStr;
    nPos = strtmp.find(delim.c_str());
    while(string::npos != nPos)
    {
        string temp = strtmp.substr(0, nPos);
        vec.push_back(temp);
        strtmp = strtmp.substr(nPos+1);
        nPos = strtmp.find(delim.c_str());
    }
    vec.push_back(strtmp);
    return vec;
}

//设定绑定的CPU
inline void bind_thread_to_cpu(std::thread& logic_thread)
{
    static std::size_t cpunum = std::thread::hardware_concurrency();
    static std::size_t cpuidx = 0;

#if PSS_PLATFORM != PLATFORM_WIN
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET((cpuidx++)%cpunum,&cpuset);
    int rc =pthread_setaffinity_np(logic_thread.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) 
    {
        PSS_LOGGER_ERROR("[bind_thread_to_cpu]Error calling pthread_setaffinity_np:{}",rc);
    }
#else
    auto mask = SetThreadAffinityMask(logic_thread.native_handle(), (cpuidx++) % cpunum);
    if (mask == 0)
    {
        PSS_LOGGER_ERROR("[bind_thread_to_cpu]Error calling SetThreadAffinityMask:{}", cpunum);
    }
#endif
}

//接口函数模板
using Logic_message_dispose_fn = std::function<int(const CMessage_Source&, std::shared_ptr<CMessage_Packet>, std::shared_ptr<CMessage_Packet>)>;
