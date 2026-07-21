#include "performance/Timer.h"

void Timer::start()
{
    startTime = std::chrono::steady_clock::now();
}

void Timer::stop()
{
    endTime = std::chrono::steady_clock::now();
}

std::chrono::nanoseconds Timer::elapsed() const
{
    return endTime - startTime;
}