#include "CommunicationService.h"

void CCommunicationService::init_communication_service(asio::io_context& io_service_context)
{
    //读取配置文件，链接服务器

    //测试定时器
    App_TimerManager::instance()->GetTimerPtr()->addTimer_loop(chrono::seconds(2), [this]()
        {
            run_check_task();
        });
}

void CCommunicationService::run_check_task()
{
    PSS_LOGGER_DEBUG("[CCommunicationService::run_check_task]ok");
}
