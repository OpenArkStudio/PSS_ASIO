// PSS_ASIO.cpp :  主工程
//

#include "ServerService.h"

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
        App_ServerService::instance()->init_servce(argv[1]);
    }
    else
    {
        App_ServerService::instance()->init_servce();
    }
    return 0;
}
