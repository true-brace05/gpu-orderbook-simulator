#pragma once

#include <chrono>

class Timer
{
public:
    void start();
    void stop();

    std::chrono::nanoseconds elapsed() const;

private:
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;
};