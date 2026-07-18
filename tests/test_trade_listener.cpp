#include <cassert>
#include <iostream>

#include "OrderBook.h"
#include "execution/ITradeListener.h"

class MockTradeListener : public ITradeListener
{
public:
    bool notified = false;

    void onTrade(const Trade& trade) override
    {
        (void)trade;
        notified = true;
    }
};

int main()
{
    OrderBook book;

    MockTradeListener listener;

    book.addTradeListener(&listener);

    Order buy{
        1,
        Side::Buy,
        OrderType::Limit,
        100,
        5,
        1
    };

    Order sell{
        2,
        Side::Sell,
        OrderType::Limit,
        100,
        5,
        2
    };

    book.addOrder(buy);
    book.addOrder(sell);

    assert(listener.notified);

    std::cout << "Trade listener test passed!\n";

    return 0;
}