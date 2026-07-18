#include <cassert>
#include <iostream>
#include <vector>

#include "OrderBook.h"
#include "replay/ReplayEngine.h"
#include "replay/readers/SyntheticReader.h"

int main()
{
    OrderBook book;
    ReplayEngine replay(book);

    std::vector<Event> events =
    {
        {
            1,
            EventType::Add,
            {1, Side::Buy, OrderType::Limit, 100, 5, 1},
            -1
        },

        {
            2,
            EventType::Add,
            {2, Side::Sell, OrderType::Limit, 100, 5, 2},
            -1
        }
    };

    SyntheticReader reader(std::move(events));

    replay.replay(reader);

    assert(book.isEmpty());

    std::cout << "Replay test passed!\n";

    return 0;
}