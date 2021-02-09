// test_pss_asio.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <gtest/gtest.h>

#include "test_Iotoio.h"

TEST(run_test_iotoio, test_iotoio)
{
    EXPECT_EQ(run_test_iotoio(), 0);
}

int main()
{
    //运行google test 测试用例
    ::testing::GTEST_FLAG(output) = "xml:Test_add.xml";
    ::testing::InitGoogleTest();

    return RUN_ALL_TESTS();
}
