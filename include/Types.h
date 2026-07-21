#pragma once

enum class Side
{
    Buy,
    Sell
};

enum class OrderType
{
    Limit,
    Market
};

enum class EventType
{
    Add,
    Cancel,
    Delete,
    ExecuteVisible,
    ExecuteHidden,
    TradingHalt
};