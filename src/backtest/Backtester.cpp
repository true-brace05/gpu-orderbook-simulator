#include "backtest/Backtester.h"

Backtester::Backtester(
    ReplayEngine& replayEngine,
    OrderBook& orderBook)
    : replayEngine(replayEngine),
      orderBook(orderBook)
{
    orderBook.addTradeListener(&statisticsCollector);
    replayEngine.addEventListener(&statisticsCollector);
}

BacktestReport Backtester::run(IEventReader& reader)
{
    auto start = std::chrono::steady_clock::now();
    replayEngine.replay(reader);
    auto end = std::chrono::steady_clock::now();
    BacktestReport report = statisticsCollector.getReport();
report.runtime = end - start;

return report;

    
}