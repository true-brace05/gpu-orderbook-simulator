#pragma once

#include <cstddef>

struct ReplayStatistics
{
    std::size_t processedEvents = 0;

    std::size_t addEvents = 0;
    std::size_t cancelEvents = 0;
    std::size_t deleteEvents = 0;

    std::size_t visibleExecutions = 0;
    std::size_t hiddenExecutions = 0;

    std::size_t tradingHalts = 0;

    double replayTimeMs = 0.0;

double eventsPerSecond = 0.0;
};