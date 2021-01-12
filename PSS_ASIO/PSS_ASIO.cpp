// PSS_ASIO.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "TcpServer.h"
#include "UdpServer.h"

#if PSS_PLATFORM == PLATFORM_WIN
#include <tchar.h>
#endif

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

    CTcpServer s1(io_context, "127.0.0.1", socket_serevr_port, 1, 102400);

    CUdpServer s2(io_context, "127.0.0.1", socket_serevr_port, 1, 1024);
    io_context.run();

}
