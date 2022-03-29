#pragma once

#include "shm_common.hpp"

#if PSS_PLATFORM == PLATFORM_WIN
#include "shm_queue_windows.hpp"
#elif PSS_PLATFORM == PLATFORM_UNIX
#include "shm_queue_Linux.hpp"
#endif

namespace shm_queue {

    class CShm_message_queue
    {
    public:
        CShm_message_queue()
        {
        };

        bool set_proc_message(const char* message_text, size_t len)
        {
            return shm_queue_->set_proc_message(message_text, len);
        };

        void recv_message(queue_recv_message_func fn_logic)
        {
            return shm_queue_->recv_message(fn_logic);
        };

        void close()
        {
            shm_queue_->close();
        };
        bool create_instance(shm_key key, size_t message_size, int message_count)
        {
#if PSS_PLATFORM == PLATFORM_WIN
            shm_queue_ = std::make_shared<CMessage_Queue_Windows>();
#elif PSS_PLATFORM == PLATFORM_UNIX
            shm_queue_ = std::make_shared<CMessage_Queue_Linux>();
#endif
            return shm_queue_->create_instance(key, message_size, message_count);
        };

        void show_message_list()
        {
            shm_queue_->show_message_list();
        };

        std::string get_error() const
        {
            return shm_queue_->get_error();
        };

        void set_error_function(queue_error_func error_func)
        {
            shm_queue_->set_error_function(error_func);
        }

        void set_close_function(queue_close_func close_func)
        {
            shm_queue_->set_close_function(close_func);
        }

    private:
        std::shared_ptr<CShm_queue_interface> shm_queue_ = nullptr;
    };
};
