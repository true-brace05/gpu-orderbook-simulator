#pragma once

#include "BacktestReport.h"
#include "replay/ReplayEngine.h"
#include "replay/readers/IEventReader.h"
#include "backtest/StatisticsCollector.h"
#include "OrderBook.h"


class Backtester
{
public:
    explicit Backtester(ReplayEngine& replayEngine, OrderBook& orderBook);

    BacktestReport run(IEventReader& reader);

private:
    
    ReplayEngine& replayEngine;
    OrderBook& orderBook;
    StatisticsCollector statisticsCollector; 
};