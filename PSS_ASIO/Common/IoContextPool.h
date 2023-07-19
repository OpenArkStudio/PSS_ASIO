#ifndef _PSS_ASIO_IO_SERVICE_POOL_H_
#define _PSS_ASIO_IO_SERVICE_POOL_H_

#include "define.h"

#include <iostream>
#include <asio.hpp>
#include <list>
#include <vector>

using CreateIoContextCallbackFunc = std::function<asio::io_context*()>; 

class CIoContextPool : private asio::noncopyable
{
public:
    CIoContextPool(std::size_t pool_size = std::thread::hardware_concurrency());

    asio::io_context* getIOContext();

    void run();
    void stop();

private:
    using io_context_ptr = std::shared_ptr<asio::io_context>;
    using io_context_work = asio::executor_work_guard<asio::io_context::executor_type>;

    std::vector<io_context_ptr> io_contexts_;
    std::list<io_context_work> works_;
    std::size_t next_io_context_;
};

using App_IoContextPool = PSS_singleton<CIoContextPool>;

extern asio::io_context* CreateIoContextFunctor();

#endif // _PSS_ASIO_IO_SERVICE_POOL_H_
