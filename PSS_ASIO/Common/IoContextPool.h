#ifndef _PSS_ASIO_IO_SERVICE_POOL_H_
#define _PSS_ASIO_IO_SERVICE_POOL_H_

#include "serverconfig.h"
#include "define.h"

#include <iostream>
#include <asio.hpp>
#include <list>
#include <vector>

using CreateIoContextCallbackFunc = std::function<asio::io_context*()>; 

//IO线程信息
class CIoThread
{
public:
    uint32 thread_id_ = 0;
    std::thread io_thread_;
};

class CIoContextPool : private asio::noncopyable
{
public:
    asio::io_context* getIOContext();

    void init(std::size_t io_size = std::thread::hardware_concurrency());
    void run();
    void stop();

private:
    std::vector<shared_ptr<CIoThread>> io_thread_list_;
    std::vector<std::shared_ptr<asio::io_context>> io_contexts_list_;
    std::size_t next_io_context_;

    using io_context_work = asio::executor_work_guard<asio::io_context::executor_type>;
    std::vector<io_context_work> works_;
};

using App_IoContextPool = PSS_singleton<CIoContextPool>;

extern asio::io_context* CreateIoContextFunctor();

#endif // _PSS_ASIO_IO_SERVICE_POOL_H_
