#pragma once

#include "backtest/BacktestReport.h"
#include "execution/ITradeListener.h"
#include "replay/IEventListener.h"

class StatisticsCollector :
    public ITradeListener,
    public IEventListener
{
public:
    void onEvent();

    void onOrderSubmitted();

    void onTrade(const Trade& trade) override;

    void onEvent(const Event& event) override;

    BacktestReport getReport() const;

private:
    BacktestReport report;
};