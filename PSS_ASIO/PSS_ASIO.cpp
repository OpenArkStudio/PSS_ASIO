// PSS_ASIO.cpp :  主工程
//

#include "ServerService.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#if PSS_PLATFORM != PLATFORM_WIN
#include <dirent.h>
#include <unistd.h>
#endif

#define BUF_SIZE 1024

int main(int argc, char* argv[])
{
    //读取配置文件参数
    if (argc > 2)
    {
        log_screen("pass asio config file param error.");
        return 0;
    }
     
    //如果存在配置文件名称，则读取指定的配置文件
    if (argc == 2)
    {
        App_ServerService::instance()->init_service(argv[1]);
    }
    else
    {
        App_ServerService::instance()->init_service("server_config.json");
    }
    return 0;
}
