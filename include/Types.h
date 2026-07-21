#pragma once

enum class Side
{
    Buy,
    Sell
};

enum class OrderType
{
    Limit,
    Market,
    Iceberg
};

enum class EventType
{
    Add,
    Cancel,
    Modify,
    Delete,
    ExecuteVisible,
    ExecuteHidden,
    TradingHalt
};
