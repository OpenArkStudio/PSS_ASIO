#pragma once

#include <string.h>
#include <cstdlib>
#include <stdio.h>
#include <cmath>
#include <sstream>
#include <thread>
#include <functional>

//自动判定操作系统
#define PLATFORM_WIN     0
#define PLATFORM_UNIX    1
#define PLATFORM_APPLE   2

#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN64__) || defined(WIN64) || defined(_WIN64)
#  define PSS_PLATFORM PLATFORM_WIN
#elif defined(__APPLE_CC__)
#  define PSS_PLATFORM PLATFORM_APPLE
#else
#  define PSS_PLATFORM PLATFORM_UNIX
#endif

const size_t shm_queue_list_size = 100;
const int shm_queue_list_count = 100;

namespace shm_queue {
#if PSS_PLATFORM == PLATFORM_WIN
    using shm_key = unsigned int;
#else
    using shm_key = key_t;
#endif

    using queue_recv_message_func = std::function<void(const char*, size_t)>;
    using queue_error_func = std::function<void(std::string)>;
    using queue_close_func = std::function<void(shm_key key)>;

    class CShm_queue_interface
    {
    public:
        virtual bool set_proc_message(const char* message_text, size_t len) = 0;
        virtual void recv_message(queue_recv_message_func fn_logic) = 0;
        virtual void close() = 0;
        virtual bool create_instance(shm_key key, size_t message_size, int message_count) = 0;
        virtual void show_message_list() = 0;
        virtual std::string get_error() const = 0;
        virtual void set_error_function(queue_error_func error_func) = 0;
        virtual void set_close_function(queue_close_func close_func) = 0;
    };
};
