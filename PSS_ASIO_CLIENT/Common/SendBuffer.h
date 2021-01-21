#pragma once

#include <string>

class CSendBuffer
{
public:
    std::string data_;
    std::size_t buffer_length_ = 0;

    void set(const char* _buffer, std::size_t _buffer_length)
    {
        data_.append(_buffer, _buffer_length);
        buffer_length_ = _buffer_length;
    }
};