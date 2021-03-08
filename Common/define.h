#pragma once

#include <math.h>
#include <type_traits>
#include <fstream>
#include <chrono>

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
    CONNECT_IO_TTY,          //IO是TTY
    CONNECT_IO_SERVER_TCP,   //IO是服务期间连接TCP
    CONNECT_IO_SERVER_UDP,   //IO是服务期间连接UDP
    CONNECT_IO_FRAME,        //来自插件间的回调
    COMMAND_UPDATE,          //来自插件更新  
    WORKTHREAD_CLOSE         //关闭当前工作线程
};

//客户端IP信息
class _ClientIPInfo
{
public:
    string  m_strClientIP = "unset ip";      //客户端的IP地址
    uint16 m_u2Port = 0;        //客户端的端口
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
    string buffer_;
    uint16 command_id_;
};

//插件内调用延迟消息数据体
class CFrame_Message_Delay
{
public:
    std::chrono::seconds delay_seconds_ = std::chrono::seconds(0); //延迟的秒数
    int timer_id_ = 0;  //定时器的id(这个必须唯一，否则会添加定时器失败)
};

//服务器间链接结构
class CConnect_IO_Info
{
public:
    uint32 server_id = 0; 
    string server_ip; 
    uint16 server_port = 0;
    uint32 packet_parse_id = 1;
    uint32 recv_size = 1024;
    uint32 send_size = 1024;
};

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
const uint16 LOGIC_MAX_FRAME_COMMAND = 0x0010;   //内部事件ID上限 

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