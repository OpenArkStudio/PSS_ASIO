#pragma once

//性能统计类，提供性能统计输出
//add by freeeyes

#include <iostream>
#include <string>
#include <chrono>
#include <functional>

class CPerformace_Check
{
public:
    CPerformace_Check(std::string name, std::function<void(const std::string, const double)> functional)
    {
        time_begin_ = std::chrono::steady_clock::now();
        func_ = functional;
    }

    ~CPerformace_Check()
    {
        double duration_millsecond = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - time_begin_).count();
        func_(name_, duration_millsecond);
    }

private:
    std::string name_;
    std::chrono::steady_clock::time_point time_begin_;
    std::function<void(const std::string, const double)> func_;
};
