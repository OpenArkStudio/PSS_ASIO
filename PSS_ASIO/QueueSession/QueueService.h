#pragma once

#include "define.h"
#include "shm_queue/shm_queue.hpp"
#include <map>

class CQueueSessionManager
{
public:
    CQueueSessionManager() = default;

    bool create_queue(shm_queue::shm_key key, size_t message_size = shm_queue_list_size, int message_count = shm_queue_list_count);
    void close();
    bool close(shm_queue::shm_key key);
    bool send_queue_message(shm_queue::shm_key key, const char* message_text, size_t len);
    bool set_close_function(shm_queue::shm_key key, shm_queue::queue_close_func close_func);
    bool set_error_function(shm_queue::shm_key key, shm_queue::queue_error_func error_func);
    bool set_recv_function(shm_queue::shm_key key, shm_queue::queue_recv_message_func fn_logic);

private:
    using hashmapqueuelist = unordered_map<shm_queue::shm_key, std::shared_ptr<shm_queue::CShm_message_queue>>;
    hashmapqueuelist queue_list_;
    std::mutex mutex_;
};

using App_QueueSessionManager = PSS_singleton<CQueueSessionManager>;
