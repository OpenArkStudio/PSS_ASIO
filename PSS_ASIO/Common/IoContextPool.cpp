#include "IoContextPool.h"
#include <stdexcept>

void IoContextRun(std::shared_ptr<asio::io_context> io) 
{
    io->run();
}

//返回当前系统支持的并发线程数
CIoContextPool::CIoContextPool(std::size_t pool_size): next_io_context_(0)
{
    if (pool_size == 0)
    {
        throw std::runtime_error("io_context_pool size is 0");
    }
    
    for (std::size_t i = 0; i < pool_size; ++i)
    {
        io_context_ptr io_context(new asio::io_context);
        io_contexts_.push_back(io_context);
        works_.push_back(asio::make_work_guard(*io_context));
    }
}

void CIoContextPool::run()
{
    // Create a pool of threads to run all of the io_contexts.
    std::vector<std::shared_ptr<std::thread> > threads;
    for (std::size_t i = 0; i < io_contexts_.size(); ++i)
    {
        std::shared_ptr<std::thread> thread(new std::thread(std::bind(&IoContextRun, io_contexts_[i])));
        threads.push_back(thread);
        
        //如果配置了CPU绑定关系
        if (App_ServerConfig::instance()->get_config_workthread().logic_thread_bind_cpu != 0)
        {
            bind_thread_to_cpu(thread.get());
        }
    }

    // Wait for all threads in the pool to exit.
    for (std::size_t i = 0; i < threads.size(); ++i)
    {
        threads[i]->join();
    }
}

asio::io_context* CIoContextPool::getIOContext()
{
    // Use a round-robin scheme to choose the next io_context to use.
    asio::io_context* io_context = io_contexts_[next_io_context_].get();
    ++next_io_context_;
    if (next_io_context_ == io_contexts_.size())
    {
        next_io_context_ = 0;
    }
    return io_context;
}

void CIoContextPool::stop()
{
    for (std::size_t i = 0; i < io_contexts_.size(); ++i)
    {
        io_contexts_[i]->stop();
    }
}

asio::io_context* CreateIoContextFunctor()
{
    return App_IoContextPool::instance()->getIOContext();
}

