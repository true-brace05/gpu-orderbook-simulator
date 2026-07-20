#pragma once

#include <cstddef>
#include <chrono>

struct BacktestReport
{
    std::size_t eventsProcessed = 0;
    std::size_t ordersSubmitted = 0;
    std::size_t tradesExecuted = 0;
    std::chrono::nanoseconds runtime{};
};