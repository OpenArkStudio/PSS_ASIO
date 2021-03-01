#include "test_buffpacket.h"

int test_buffpacket_number_data()
{
    //²âÊÔÊı×ÖÓÃÀı
    int ret = 0;
    auto buff_packet = std::make_shared<CBuffPacket>();

    //²åÈëÁ÷
    uint8 u8_data = 1;
    uint16 u16_data = 2;
    uint32 u32_data = 3;
    uint64 u64_data = 4;
    int8 n8_data = -1;
    int16 n16_data = -2;
    int32 n32_data = -3;
    int64 n64_data = -4;
    float32 f32_data = 3.14f;
    float64 f64_data = -3.14l;

    //¶ÁÈ¡Á÷
    uint8 u8_data_tag = 0;
    uint16 u16_data_tag = 0;
    uint32 u32_data_tag = 0;
    uint64 u64_data_tag = 0;
    int8 n8_data_tag = 0;
    int16 n16_data_tag = 0;
    int32 n32_data_tag = 0;
    int64 n64_data_tag = 0;
    float32 f32_data_tag = 0.0f;
    float64 f64_data_tag = 0.0l;

    (*buff_packet) << u8_data;
    (*buff_packet) << u16_data;
    (*buff_packet) << u32_data;
    (*buff_packet) << u64_data;
    (*buff_packet) << n8_data;
    (*buff_packet) << n16_data;
    (*buff_packet) << n32_data;
    (*buff_packet) << n64_data;
    (*buff_packet) << f32_data;
    (*buff_packet) << f64_data;

    //ºË¶Ô²âÊÔ½á¹û
    (*buff_packet) >> u8_data_tag;
    if (u8_data_tag != u8_data)
    {
        std::cout << "[test_buffpacket_number_data]u8_data_tag error." << std::endl;
        ret = 1;
    }
    (*buff_packet) >> u16_data_tag;
    if (u16_data_tag != u16_data)
    {
       std::cout << "[test_buffpacket_number_data]u16_data_tag error." << std::endl;
       ret = 1;
    }
    (*buff_packet) >> u32_data_tag;
    if (u32_data_tag != u32_data)
    {
        std::cout << "[test_buffpacket_number_data]u32_data_tag error." << std::endl;
        ret = 1;
    }
    (*buff_packet) >> u64_data_tag;
    if (u64_data_tag != u64_data)
    {
        std::cout << "[test_buffpacket_number_data]u64_data_tag error." << std::endl;
        ret = 1;
    }
    (*buff_packet) >> n8_data_tag;
    if (n8_data_tag != n8_data)
    {
        std::cout << "[test_buffpacket_number_data]n8_data_tag error." << std::endl;
        ret = 1;
    }
    (*buff_packet) >> n16_data_tag;
    if (n16_data_tag != n16_data)
    {
        std::cout << "[test_buffpacket_number_data]n16_data_tag error." << std::endl;
        ret = 1;
    }
    (*buff_packet) >> n32_data_tag;
    if (n32_data_tag != n32_data)
    {
        std::cout << "[test_buffpacket_number_data]n32_data_tag error." << std::endl;
        ret = 1;
    }
    (*buff_packet) >> n64_data_tag;
    if (n64_data_tag != n64_data)
    {
        std::cout << "[test_buffpacket_number_data]n64_data_tag error." << std::endl;
        ret = 1;
    }
    (*buff_packet) >> f32_data_tag;
    if (f32_data_tag != f32_data)
    {
        std::cout << "[test_buffpacket_number_data]f32_data_tag error." << std::endl;
        ret = 1;
    }
    (*buff_packet) >> f64_data_tag;
    if (f64_data_tag != f64_data)
    {
        std::cout << "[test_buffpacket_number_data]f64_data_tag error." << std::endl;
        ret = 1;
    }

    return ret;
}

int test_buffpacket_string_data()
{
    //²âÊÔ×Ö·û´®ÓÃÀı
    int ret = 0;
    auto buff_packet = std::make_shared<CBuffPacket>();

    std::string string_data = "freeeyes";
    (*buff_packet) << string_data;

    std::string string_data_tag = "";
    (*buff_packet) >> string_data_tag;
    if (string_data_tag != string_data_tag)
    {
        std::cout << "[test_buffpacket_string_data]string_data_tag error." << std::endl;
        ret = 1;
    }

    return ret;
}

int test_offset_number_data()
{
    //²âÊÔÊı×ÖÓÃÀı
    int ret = 0;
    auto buff_packet = std::make_shared<CBuffPacket>();

    //²åÈëÁ÷
    uint8 u8_data = 1;
    uint16 u16_data = 2;
    uint32 u32_data = 3;
    uint64 u64_data = 4;

    (*buff_packet) << u8_data;
    (*buff_packet) << u16_data;
    (*buff_packet) << u32_data;
    (*buff_packet) << u64_data;

    //²âÊÔÆ«ÒÆÈ¡Öµ
    buff_packet->read_offset(3);

    uint8 u8_data_tag = 0;
    uint32 u32_data_tag = 0;

    (*buff_packet) >> u32_data_tag;
    if (u32_data_tag != u32_data)
    {
        std::cout << "[test_offset_number_data]u32_data_tag error." << std::endl;
        ret = 1;
    }

    buff_packet->read_offset(-7);
    (*buff_packet) >> u8_data_tag;
    if (u8_data_tag != u8_data)
    {
        std::cout << "[test_offset_number_data]u8_data_tag error." << std::endl;
        ret = 1;
    }

    return ret;
}

