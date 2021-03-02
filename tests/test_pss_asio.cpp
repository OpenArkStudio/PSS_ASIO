// test_pss_asio.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <gtest/gtest.h>

#include "test_buffpacket.h"
#include "test_Iotoio.h"

TEST(run_test_iotoio, test_iotoio)
{
    EXPECT_EQ(run_test_iotoio(), 0);
}

TEST(run_test_buffpacket, test_buffpacket_number)
{
    EXPECT_EQ(test_buffpacket_number_data(), 0);
}

TEST(run_test_buffpacket, test_buffpacket_string)
{
    EXPECT_EQ(test_buffpacket_string_data(), 0);
}

TEST(run_test_buffpacket, test_buffpacket_offset)
{
    EXPECT_EQ(test_offset_number_data(), 0);
}

TEST(run_test_buffpacket, test_buffpacket_net_order)
{
    EXPECT_EQ(test_net_order_data(), 0);
}


int main(int argc, char* argv[])
{
    //运行google test 测试用例
    ::testing::GTEST_FLAG(output) = "xml:Test_Pss_Asio.xml";
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
