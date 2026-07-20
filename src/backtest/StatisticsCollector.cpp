#include "backtest/StatisticsCollector.h"

void StatisticsCollector::onEvent()
{
    ++report.eventsProcessed;
}

void StatisticsCollector::onOrderSubmitted()
{
    ++report.ordersSubmitted;
}

void StatisticsCollector::onTrade(const Trade&)
{
    ++report.tradesExecuted;
}

BacktestReport StatisticsCollector::getReport() const
{
    return report;
}

void StatisticsCollector::onEvent(const Event&)
{
    ++report.eventsProcessed;
}