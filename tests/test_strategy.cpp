#include <cassert>
#include <iostream>
#include <vector>

#include "OrderBook.h"
#include "replay/ReplayEngine.h"
#include "replay/readers/SyntheticReader.h"
#include "strategy/PassiveBuyer.h"

int main()
{
    OrderBook book;

    ReplayEngine replay(book);

    PassiveBuyer strategy;

    replay.setStrategy(&strategy);

    std::vector<Event> events =
    {
        {
            1,
            EventType::Add,
            {1, Side::Sell, OrderType::Limit, 100, 5, 1},
            -1
        }
    };

    SyntheticReader reader(std::move(events));

    replay.replay(reader);

    auto generatedOrder = book.getOrder(1000001);

    assert(generatedOrder.has_value());

    assert(generatedOrder->side == Side::Buy);
    assert(generatedOrder->price == 100);
    assert(generatedOrder->quantity == 1);

    std::cout << "Strategy test passed!\n";

    return 0;
}