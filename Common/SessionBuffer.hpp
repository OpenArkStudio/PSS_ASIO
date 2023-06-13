#pragma once

#include <iostream>
#include <string>
#include <memory>

class CSessionBuffer
{
public:
    CSessionBuffer() = default;

    void Init(size_t _size)
    {
        //std::cout << "buffer is create" << std::endl;
        buffer_.reset(new char[_size], [](char* _data) {
            //std::cout << "buffer is delete" << std::endl;
            delete[] _data;
            });

        write_size_ = 0;
        max_buff_size_ = _size;
    }

    bool set_write_data(size_t length)
    {
        if (write_size_ + length > max_buff_size_)
        {
            return false;
        }
        else
        {
            write_size_ += length;
            return true;
        }
    }

    char* get_curr_write_ptr()
    {
        return buffer_.get() + write_size_;
    }

    size_t get_buffer_size()
    {
        return max_buff_size_ - write_size_;
    }

    char* read()
    {
        return  buffer_.get();
    }

    size_t get_write_size()
    {
        return write_size_;
    }

    void move(size_t _length)
    {
        if (_length >= write_size_)
        {
            write_size_ = 0;
        }
        else
        {
            size_t move_length = write_size_ - _length;
            std::memmove(buffer_.get(), buffer_.get() + _length, move_length);
            write_size_ = move_length;
        }
    }

    CSessionBuffer& operator = (const CSessionBuffer&) = delete;

private:
    std::shared_ptr<char> buffer_;
    size_t write_size_ = 0;               //写入的当前长度
    size_t max_buff_size_ = 0;            //最大缓存长度
};

