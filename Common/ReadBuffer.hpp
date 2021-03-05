#include "define.h"
#if PSS_PLATFORM == PLATFORM_WIN
#include<WinSock2.h>
#pragma comment(lib, "wsock32.lib")
#else
#include <arpa/inet.h>
#endif

inline uint64 ntohll_uint64(uint64 val)
{
    return (((uint64)ntohl((int)((val << 32) >> 32))) << 32) | (unsigned int)ntohl((int)(val >> 32));
}

inline int64 ntohll_int64(int64 val)
{
    return (((int64)ntohl((int)((val << 32) >> 32))) << 32) | (unsigned int)ntohl((int)(val >> 32));
}

class CReadBuffer
{
public:
    CReadBuffer(const std::string* buffer) : buffer_(buffer)
    {
        write_ptr_ = (uint32)buffer->size();
    }

    CReadBuffer& operator >> (uint8& u1Data)
    {
        u1Data = 0;

        if (write_ptr_ - read_ptr_ >= sizeof(u1Data))
        {
            u1Data = *(uint8*)read_ptr();
            read_ptr_ += (uint32)sizeof(u1Data);
        }

        return *this;
    };

    CReadBuffer& operator >> (uint16& u2Data)
    {
        u2Data = 0;

        if (write_ptr_ - read_ptr_ >= (uint32)sizeof(u2Data))
        {
            std::memcpy(&u2Data, read_ptr(), (uint32)sizeof(uint16));
            read_ptr_ += (uint32)sizeof(u2Data);
        }

        if (true == is_net_sort_)
        {
            //转化为本地字节序
            u2Data = ntohs(u2Data);
        }

        return *this;
    };

    CReadBuffer& operator >> (uint32& u4Data)
    {
        u4Data = 0;

        if (write_ptr_ - read_ptr_ >= (uint32)sizeof(u4Data))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&u4Data, read_ptr(), (uint32)sizeof(u4Data));
            read_ptr_ += (uint32)sizeof(u4Data);
        }

        if (true == is_net_sort_)
        {
            //转化为本地字节序
            u4Data = ntohl(u4Data);
        }

        return *this;
    };

    CReadBuffer& operator >> (uint64& u8Data)
    {
        u8Data = 0;

        if (write_ptr_ - read_ptr_ >= (uint32)sizeof(u8Data))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&u8Data, read_ptr(), (uint32)sizeof(u8Data));
            read_ptr_ += (uint32)sizeof(u8Data);
        }

        if (true == is_net_sort_)
        {
            //转化为本地字节序
            u8Data = ntohll_uint64(u8Data);
        }

        return *this;
    };

    CReadBuffer& operator >> (int8& n1Data)
    {
        n1Data = 0;

        if (write_ptr_ - read_ptr_ >= sizeof(n1Data))
        {
            n1Data = *(uint8*)read_ptr();
            read_ptr_ += (uint32)sizeof(n1Data);
        }

        return *this;
    };

    CReadBuffer& operator >> (int16& n2Data)
    {
        n2Data = 0;

        if (write_ptr_ - read_ptr_ >= (int16)sizeof(n2Data))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&n2Data, read_ptr(), (uint32)sizeof(int16));
            read_ptr_ += (uint32)sizeof(n2Data);
        }

        if (true == is_net_sort_)
        {
            //转化为本地字节序
            n2Data = ntohs(n2Data);
        }

        return *this;
    };

    CReadBuffer& operator >> (int32& n4Data)
    {
        n4Data = 0;

        if (write_ptr_ - read_ptr_ >= (int32)sizeof(n4Data))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&n4Data, read_ptr(), (uint32)sizeof(int32));
            read_ptr_ += (uint32)sizeof(n4Data);
        }

        if (true == is_net_sort_)
        {
            //转化为本地字节序
            n4Data = ntohl(n4Data);
        }


        return *this;
    };

    CReadBuffer& operator >> (int64& n8Data)
    {
        n8Data = 0;

        if (write_ptr_ - read_ptr_ >= (int32)sizeof(int64))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&n8Data, read_ptr(), (uint32)sizeof(int64));
            read_ptr_ += (uint32)sizeof(n8Data);
        }

        if (true == is_net_sort_)
        {
            //转化为本地字节序
            n8Data = ntohll_int64(n8Data);
        }

        return *this;
    };

    CReadBuffer& operator >> (float32& f4Data)
    {
        f4Data = 0;

        if (write_ptr_ - read_ptr_ >= (int32)sizeof(f4Data))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&f4Data, read_ptr(), (uint32)sizeof(float32));
            read_ptr_ += (uint32)sizeof(f4Data);
        }

        return *this;
    };

    CReadBuffer& operator >> (float64& f8Data)
    {
        f8Data = 0;

        if (write_ptr_ - read_ptr_ >= (int32)sizeof(f8Data))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&f8Data, read_ptr(), (uint32)sizeof(float64));
            read_ptr_ += (uint32)sizeof(f8Data);
        }

        return *this;
    };

    CReadBuffer& operator >> (std::string& str)
    {
        uint32 u4Len = 0;
        (*this) >> u4Len;

        if (u4Len && write_ptr_ - read_ptr_ >= u4Len)
        {
            const char* pData = read_ptr();
            read_ptr_ += u4Len;
            str = std::string(pData, u4Len);
        }

        return *this;
    };

    void read_data(char* data, uint32 size, uint32 length)
    {
        if (size < length)
        {
            return;
        }

        if (length > write_ptr_ - read_ptr_)
        {
            length = write_ptr_ - read_ptr_;
        }

        std::memcpy(data, read_ptr(), length);
        read_ptr_ += length;
    };

    void read_data_to_string(std::string& data, int length = -1)
    {
        if (length == -1)
        {
            length = write_ptr_ - read_ptr_;
        }

        if (length > (int)(write_ptr_ - read_ptr_))
        {
            length = write_ptr_ - read_ptr_;
        }

        data.append(read_ptr(), length);
        read_ptr_ += length;
    };

    bool read_offset(int offset_size)
    {
        if (offset_size <= (int)(write_ptr_ - read_ptr_) && offset_size > 0)
        {
            read_ptr_ += offset_size;
            return true;
        }
        else if (offset_size < 0)
        {
            if (read_ptr_ + offset_size > 0)
            {
                read_ptr_ -= offset_size;
            }
            else
            {
                read_ptr_ = 0;
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    void set_net_sort(bool net_sort)
    {
        is_net_sort_ = net_sort;
    };

private:
    const char* read_ptr()
    {
        return buffer_->c_str() + read_ptr_;
    };

    const std::string* buffer_ = nullptr;
    uint32 read_ptr_ = 0;
    uint32 write_ptr_ = 0;
    bool is_net_sort_ = false;

};