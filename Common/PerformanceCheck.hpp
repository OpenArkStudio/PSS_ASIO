#pragma once

//性能统计类，提供性能统计输出
//add by freeeyes

#include <iostream>
#include <string>
#include <chrono>
#include <functional>
#include<random>
#include<time.h>

//一个采样器
class CRandom_Sample
{
public:
    CRandom_Sample() = delete;

    explicit CRandom_Sample(int sample_ratio)
    {
        sample_ratio_ = sample_ratio;
    }

    bool Get_Random()
    {
        if (0 <= dis_(gen_) && sample_ratio_ > dis_(gen_))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

private:
    std::random_device rd_;
    std::mt19937 gen_{ rd_() };
    std::uniform_int_distribution<int> dis_{0, 100};
    int sample_ratio_;
};

class CPerformance_Check
{
public:
    CPerformance_Check() = delete;

    explicit CPerformance_Check(CRandom_Sample& random_sample, std::string name, std::function<void(const std::string, const double)> functional)
    {
        if (random_sample.Get_Random())
        {
            is_callback_ = true;
            time_begin_ = std::chrono::steady_clock::now();
        }
        func_ = functional;
    }

    explicit CPerformance_Check(std::string name, std::function<void(const std::string, const double)> functional)
    {
        time_begin_ = std::chrono::steady_clock::now();
        func_ = functional;
    }

    ~CPerformance_Check()
    {
        if (is_callback_)
        {
            double duration_millsecond = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - time_begin_).count();
            func_(name_, duration_millsecond);
        }
    }

private:
    std::string name_;
    std::chrono::steady_clock::time_point time_begin_;
    std::function<void(const std::string, const double)> func_;
    bool is_callback_ = true;
};
