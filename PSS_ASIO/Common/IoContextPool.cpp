#include "IoContextPool.h"
#include <stdexcept>

void CIoContextPool::init(std::size_t io_size)
{
    next_io_context_ = 0;

    if (io_size == 0)
    {
        throw std::runtime_error("io_context_pool size is 0");
    }

    for (std::size_t i = 0; i < io_size; ++i)
    {
        auto io_context = std::make_shared<asio::io_context>(1);
        io_contexts_list_.push_back(io_context);
        works_.push_back(asio::make_work_guard(*io_context));
    }
}

void CIoContextPool::run()
{
    // Create a pool of threads to run all of the io_contexts.
    int thread_index = 0;
    
    for (const auto& io_context : io_contexts_list_)
    {
        auto io_thread = std::make_shared<CIoThread>();
        io_thread->thread_id_ = thread_index;
        auto curr_thread_id = thread_index;
        io_thread->io_thread_ = std::thread([&, curr_thread_id]()
            {
                PSS_LOGGER_DEBUG("[CIoContextPool::run]curr_thread_id = {0} io_context is begin run", curr_thread_id);
                io_context->run();
                PSS_LOGGER_DEBUG("[CIoContextPool::run]curr_thread_id = {0} io_context is stop", curr_thread_id);
            });

        io_thread_list_.push_back(io_thread);
        
        //如果配置了CPU绑定关系
        if (App_ServerConfig::instance()->get_config_workthread().logic_thread_bind_cpu_ != 0)
        {
            bind_thread_to_cpu(io_thread->io_thread_);
        }

        thread_index++;
    }

    // Wait for all threads in the pool to exit.
    auto thread_pos = 0;
    for (const auto& io_thread : io_thread_list_)
    {
        io_thread->io_thread_.join();
        thread_pos++;
    }
    
    io_thread_list_.clear();
}

asio::io_context* CIoContextPool::getIOContext()
{
    // Use a round-robin scheme to choose the next io_context to use.
    auto io_context = io_contexts_list_[next_io_context_].get();
    ++next_io_context_;
    if (next_io_context_ == io_contexts_list_.size())
    {
        next_io_context_ = 1;
    }
    return io_context;
}

void CIoContextPool::stop()
{
    for (auto iter = works_.begin(); iter != works_.end(); iter++)
    {
        iter->reset();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    for (const auto& io_context : io_contexts_list_)
    {
        io_context->stop();
    }
    io_contexts_list_.clear();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

asio::io_context* CreateIoContextFunctor()
{
    return App_IoContextPool::instance()->getIOContext();
}

