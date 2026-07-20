#include "execution/ExecutionEngine.h"

std::optional<Fill> ExecutionEngine::processTrade(
    const Trade& trade,
    Side strategySide)
{
    Fill fill;

    fill.side = strategySide;
    fill.price = trade.price;
    fill.quantity = trade.quantity;
    fill.timestamp = trade.timestamp;

    for (IFillListener* listener : fillListeners)
{
    listener->onFill(fill);
}

    return fill;
}

void ExecutionEngine::addFillListener(IFillListener* listener)
{
    fillListeners.push_back(listener);
}

void ExecutionEngine::onTrade(const Trade& trade)
{
    auto fill = processTrade(trade, Side::Buy);

    if (!fill.has_value())
    {
        return;
    }

    for (IFillListener* listener : fillListeners)
    {
        listener->onFill(fill.value());
    }
}