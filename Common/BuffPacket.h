#pragma once

#include "define.h"

class CBuffPacket
{
public:
    CBuffPacket& operator >> (uint8& u1Data);
    CBuffPacket& operator >> (uint16& u2Data);
    CBuffPacket& operator >> (uint32& u4Data);
    CBuffPacket& operator >> (uint64& u8Data);
    CBuffPacket& operator >> (int8& n1Data);
    CBuffPacket& operator >> (int16& n2Data);
    CBuffPacket& operator >> (int32& n4Data);

    CBuffPacket& operator >> (float32& f4Data);
    CBuffPacket& operator >> (float64& f8Data);
    CBuffPacket& operator >> (std::string& str);

    CBuffPacket& operator << (uint8 u1Data);
    CBuffPacket& operator << (uint16 u2Data);
    CBuffPacket& operator << (uint32 u4Data);
    CBuffPacket& operator << (uint64 u8Data);
    CBuffPacket& operator << (int8 n1Data);
    CBuffPacket& operator << (int16 n2Data);
    CBuffPacket& operator << (int32 n4Data);

    CBuffPacket& operator << (float32 f4Data);
    CBuffPacket& operator << (float64 f8Data);
    CBuffPacket& operator << (std::string& str);

    const char* ReadPtr();

    void clear();

    std::string& get_buffer();

private:
    std::string buffer_;
    uint32 read_ptr_ = 0;
    uint32 write_ptr_ = 0;
};

