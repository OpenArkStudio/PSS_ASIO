#pragma once

#include "define.h"

class CBuffPacket
{
public:
    CBuffPacket& operator >> (uint8& u1Data)
    {
        u1Data = 0;

        if (write_ptr_ - read_ptr_ >= sizeof(u1Data))
        {
            u1Data = *(uint8*)ReadPtr();
            read_ptr_ += (uint32)sizeof(u1Data);
        }

        return *this;
    };

    CBuffPacket& operator >> (uint16& u2Data)
    {
        u2Data = 0;

        if (write_ptr_ - read_ptr_ >= (uint32)sizeof(u2Data))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&u2Data, ReadPtr(), (uint32)sizeof(uint16));
            read_ptr_ += (uint32)sizeof(u2Data);
        }

        return *this;
    };

    CBuffPacket& operator >> (uint32& u4Data)
    {
        u4Data = 0;

        if (write_ptr_ - read_ptr_ >= (uint32)sizeof(u4Data))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&u4Data, ReadPtr(), (uint32)sizeof(u4Data));
            read_ptr_ += (uint32)sizeof(u4Data);
        }

        return *this;
    };

    CBuffPacket& operator >> (uint64& u8Data)
    {
        u8Data = 0;

        if (write_ptr_ - read_ptr_ >= (uint32)sizeof(u8Data))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&u8Data, ReadPtr(), (uint32)sizeof(u8Data));
            read_ptr_ += (uint32)sizeof(u8Data);
        }

        return *this;
    };

    CBuffPacket& operator >> (int8& n1Data)
    {
        n1Data = 0;

        if (write_ptr_ - read_ptr_ >= sizeof(n1Data))
        {
            n1Data = *(uint8*)ReadPtr();
            read_ptr_ += (uint32)sizeof(n1Data);
        }

        return *this;
    };

    CBuffPacket& operator >> (int16& n2Data)
    {
        n2Data = 0;

        if (write_ptr_ - read_ptr_ >= (int16)sizeof(n2Data))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&n2Data, ReadPtr(), (uint32)sizeof(int16));
            read_ptr_ += (uint32)sizeof(n2Data);
        }

        return *this;
    };

    CBuffPacket& operator >> (int32& n4Data)
    {
        n4Data = 0;

        if (write_ptr_ - read_ptr_ >= (int32)sizeof(n4Data))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&n4Data, ReadPtr(), (uint32)sizeof(int32));
            read_ptr_ += (uint32)sizeof(n4Data);
        }

        return *this;
    };

    CBuffPacket& operator >> (float32& f4Data)
    {
        f4Data = 0;

        if (write_ptr_ - read_ptr_ >= (int32)sizeof(f4Data))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&f4Data, ReadPtr(), (uint32)sizeof(float32));
            read_ptr_ += (uint32)sizeof(f4Data);
        }

        return *this;
    };

    CBuffPacket& operator >> (float64& f8Data)
    {
        f8Data = 0;

        if (write_ptr_ - read_ptr_ >= (int32)sizeof(f8Data))
        {
            //把网络字节序，转换为主机字节序
            std::memcpy(&f8Data, ReadPtr(), (uint32)sizeof(float64));
            read_ptr_ += (uint32)sizeof(f8Data);
        }

        return *this;
    };

    CBuffPacket& operator >> (std::string& str)
    {
        uint32 u4Len = 0;
        (*this) >> (u4Len);

        if (u4Len && write_ptr_ - read_ptr_ >= u4Len)
        {
            const char* pData = ReadPtr();
            read_ptr_ += u4Len;
            str = std::string(pData, u4Len);
        }

        return *this;
    };

    CBuffPacket& operator << (uint8 u1Data)
    {
        buffer_.assign((char*)&u1Data, sizeof(u1Data));
        write_ptr_ += sizeof(u1Data);
        return *this;
    };

    CBuffPacket& operator << (uint16 u2Data)
    {
        buffer_.assign((char*)&u2Data, sizeof(u2Data));
        write_ptr_ += sizeof(u2Data);
        return *this;
    };

    CBuffPacket& operator << (uint32 u4Data)
    {
        buffer_.assign((char*)&u4Data, sizeof(u4Data));
        write_ptr_ += sizeof(u4Data);
        return *this;
    };

    CBuffPacket& operator << (uint64 u8Data)
    {
        buffer_.assign((char*)&u8Data, sizeof(u8Data));
        write_ptr_ += sizeof(u8Data);
        return *this;
    };

    CBuffPacket& operator << (int8 n1Data)
    {
        buffer_.assign((char*)&n1Data, sizeof(n1Data));
        write_ptr_ += sizeof(n1Data);
        return *this;
    };

    CBuffPacket& operator << (int16 n2Data)
    {
        buffer_.assign((char*)&n2Data, sizeof(n2Data));
        write_ptr_ += sizeof(n2Data);
        return *this;
    };

    CBuffPacket& operator << (int32 n4Data)
    {
        buffer_.assign((char*)&n4Data, sizeof(n4Data));
        write_ptr_ += sizeof(n4Data);
        return *this;
    };

    CBuffPacket& operator << (float32 f4Data)
    {
        buffer_.assign((char*)&f4Data, sizeof(f4Data));
        write_ptr_ += sizeof(f4Data);
        return *this;
    };

    CBuffPacket& operator << (float64 f8Data)
    {
        buffer_.assign((char*)&f8Data, sizeof(f8Data));
        write_ptr_ += sizeof(f8Data);
        return *this;
    };

    CBuffPacket& operator << (std::string& str)
    {
        *this << str.length();

        buffer_.assign(str.c_str(), str.length());
        write_ptr_ += sizeof(str.length());
        return *this;
    };

    const char* ReadPtr()
    {
        return buffer_.c_str() + read_ptr_;
    };

    void clear()
    {
        buffer_.clear();
        read_ptr_ = 0;
        write_ptr_ = 0;
    };

    std::string& get_buffer()
    {
        return buffer_;
    };

private:
    std::string buffer_;
    uint32 read_ptr_ = 0;
    uint32 write_ptr_ = 0;
};

