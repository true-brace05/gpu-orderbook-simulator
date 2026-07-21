#include "marketdata/LOBSTERValidator.h"

#include <algorithm>
#include <iostream>

bool LOBSTERValidator::validate(
    const OrderBookSnapshot& ours,
    const OrderBookSnapshot& expected)
{
    if (ours.bids.size() != expected.bids.size())
        return false;

    if (ours.asks.size() != expected.asks.size())
        return false;

    for (std::size_t i = 0; i < ours.bids.size(); ++i)
    {
        if (ours.bids[i].price != expected.bids[i].price)
            return false;

        if (ours.bids[i].quantity != expected.bids[i].quantity)
            return false;
    }

    for (std::size_t i = 0; i < ours.asks.size(); ++i)
    {
        if (ours.asks[i].price != expected.asks[i].price)
            return false;

        if (ours.asks[i].quantity != expected.asks[i].quantity)
            return false;
    }

    return true;
}

void LOBSTERValidator::printDifferences(
    const OrderBookSnapshot& ours,
    const OrderBookSnapshot& expected)
{
    std::cout << "\n========== VALIDATION REPORT ==========\n\n";

    std::size_t bidLevels =
        std::min(ours.bids.size(), expected.bids.size());

    for (std::size_t i = 0; i < bidLevels; ++i)
    {
        if (ours.bids[i].price != expected.bids[i].price ||
            ours.bids[i].quantity != expected.bids[i].quantity)
        {
            std::cout
                << "Bid Level " << i + 1 << '\n'
                << "Expected : "
                << expected.bids[i].price
                << " @ "
                << expected.bids[i].quantity
                << '\n'
                << "Actual   : "
                << ours.bids[i].price
                << " @ "
                << ours.bids[i].quantity
                << "\n\n";
        }
    }

    std::size_t askLevels =
        std::min(ours.asks.size(), expected.asks.size());

    for (std::size_t i = 0; i < askLevels; ++i)
    {
        if (ours.asks[i].price != expected.asks[i].price ||
            ours.asks[i].quantity != expected.asks[i].quantity)
        {
            std::cout
                << "Ask Level " << i + 1 << '\n'
                << "Expected : "
                << expected.asks[i].price
                << " @ "
                << expected.asks[i].quantity
                << '\n'
                << "Actual   : "
                << ours.asks[i].price
                << " @ "
                << ours.asks[i].quantity
                << "\n\n";
        }
    }
}