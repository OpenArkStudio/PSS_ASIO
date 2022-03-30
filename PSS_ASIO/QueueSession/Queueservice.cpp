#include "QueueService.h"


bool CQueueSessionManager::create_queue(shm_queue::shm_key key, size_t message_size, int message_count)
{
    std::lock_guard <std::mutex> lock(mutex_);
    auto f = queue_list_.find(key);
    if (f != queue_list_.end())
    {
        return true;
    }

    auto message_queue = std::make_shared<shm_queue::CShm_message_queue>();
    if (false == message_queue->create_instance(key, message_size, message_count))
    {
        PSS_LOGGER_DEBUG("[CQueueSessionManager::create_queue]err={0}", message_queue->get_error());
        return false;
    }
    else
    {
        queue_list_[key] = message_queue;
        return true;
    }
}

void CQueueSessionManager::close()
{
    std::lock_guard <std::mutex> lock(mutex_);
    for (const auto& f : queue_list_)
    {
        f.second->close();
    }
}

bool CQueueSessionManager::close(shm_queue::shm_key key)
{
    std::lock_guard <std::mutex> lock(mutex_);
    auto f = queue_list_.find(key);
    if (f != queue_list_.end())
    {
        f->second->close();
        queue_list_.erase(f);
        return true;
    }
    else
    {
        return false;
    }
}

bool CQueueSessionManager::send_queue_message(shm_queue::shm_key key, const char* message_text, size_t len)
{
    std::lock_guard <std::mutex> lock(mutex_);
    auto f = queue_list_.find(key);
    if (f != queue_list_.end())
    {
        f->second->set_proc_message(message_text, len);
        return true;
    }
    else
    {
        return false;
    }
}

bool CQueueSessionManager::set_close_function(shm_queue::shm_key key, shm_queue::queue_close_func close_func)
{
    std::lock_guard <std::mutex> lock(mutex_);
    auto f = queue_list_.find(key);
    if (f != queue_list_.end())
    {
        f->second->set_close_function(close_func);
        return true;
    }
    else
    {
        return false;
    }
}

bool CQueueSessionManager::set_error_function(shm_queue::shm_key key, shm_queue::queue_error_func error_func)
{
    std::lock_guard <std::mutex> lock(mutex_);
    auto f = queue_list_.find(key);
    if (f != queue_list_.end())
    {
        f->second->set_error_function(error_func);
        return true;
    }
    else
    {
        return false;
    }
}

bool CQueueSessionManager::set_recv_function(shm_queue::shm_key key, shm_queue::queue_recv_message_func fn_logic)
{
    std::lock_guard <std::mutex> lock(mutex_);
    auto f = queue_list_.find(key);
    if (f != queue_list_.end())
    {
        f->second->recv_message(fn_logic);
        return true;
    }
    else
    {
        return false;
    }
}
