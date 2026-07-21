#include <iostream>

#include "OrderBook.h"

namespace
{

bool expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return false;
    }

    return true;
}

} // namespace

int main()
{
    OrderBook book;
    book.setVerbose(false);

    book.addOrder({1, Side::Sell, OrderType::Iceberg, 100, 2, 1, 2, 3});
    book.addOrder({2, Side::Sell, OrderType::Limit, 100, 2, 2});

    auto iceberg = book.getOrder(1);

    if (!expect(iceberg.has_value(), "Iceberg order was not added") ||
        !expect(iceberg->quantity == 2, "Initial visible quantity is incorrect") ||
        !expect(iceberg->reserveQuantity == 3, "Initial reserve quantity is incorrect"))
    {
        return 1;
    }

    book.addOrder({3, Side::Buy, OrderType::Market, 0, 2, 3});

    iceberg = book.getOrder(1);

    if (!expect(iceberg.has_value(), "Iceberg order was not replenished") ||
        !expect(iceberg->quantity == 2, "Replenished visible quantity is incorrect") ||
        !expect(iceberg->reserveQuantity == 1, "Reserve was not reduced after replenishment"))
    {
        return 1;
    }

    book.addOrder({4, Side::Buy, OrderType::Market, 0, 2, 4});

    if (!expect(!book.hasOrder(2), "Same-price order did not receive priority") ||
        !expect(book.hasOrder(1), "Iceberg order was matched before the queued order"))
    {
        return 1;
    }

    book.addOrder({5, Side::Buy, OrderType::Market, 0, 2, 5});

    iceberg = book.getOrder(1);

    if (!expect(iceberg.has_value(), "Final iceberg tranche was not replenished") ||
        !expect(iceberg->quantity == 1, "Final visible tranche is incorrect") ||
        !expect(iceberg->reserveQuantity == 0, "Final reserve quantity is incorrect"))
    {
        return 1;
    }

    book.addOrder({6, Side::Buy, OrderType::Market, 0, 1, 6});

    if (!expect(book.isEmpty(), "Iceberg order remained after its final fill"))
    {
        return 1;
    }

    OrderBook limitBook;
    limitBook.setVerbose(false);

    limitBook.addOrder({10, Side::Sell, OrderType::Limit, 101, 5, 10});
    limitBook.addOrder({11, Side::Buy, OrderType::Limit, 101, 3, 11});

    const auto remainingLimitOrder = limitBook.getOrder(10);

    if (!expect(remainingLimitOrder.has_value(), "Normal limit order was removed") ||
        !expect(remainingLimitOrder->quantity == 2,
                "Normal limit order matching changed"))
    {
        return 1;
    }

    std::cout << "Iceberg order test passed!\n";

    return 0;
}
