// PSS_ASIO.cpp :  主工程
//

#include "ServerService.h"
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 1024

string GetNameByPid(pid_t pid) 
{
    char proc_pid_path[BUF_SIZE] = {0};
    char buf[BUF_SIZE] = {0};
    char task_name[BUF_SIZE] = {0};
    string strTaskName = "";

    sprintf(proc_pid_path, "/proc/%d/status", pid);
    FILE* fp = fopen(proc_pid_path, "r");
    if(NULL != fp)
    {
        if( fgets(buf, BUF_SIZE-1, fp) == nullptr)
        {
            fclose(fp);
        }
        else
        {
            fclose(fp);
            sscanf(buf, "%*s %s", task_name);
        }
        
        strTaskName = task_name;
    }

    return strTaskName;
}

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
        string strAppName = GetNameByPid(getpid());
        string strCfgFile = strAppName + ".json"; 
        App_ServerService::instance()->init_service(strCfgFile);
    }
    return 0;
}
