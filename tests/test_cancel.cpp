#include <cassert>
#include <iostream>

#include "OrderBook.h"
#include "testFramework.h"


void testCancelExistingOrder()
{
    printTestHeader("Cancel Existing Order");

    // Arrange
    OrderBook book;

    book.addOrder({1, Side::Buy, OrderType::Limit, 100, 5, 1});
    book.addOrder({2, Side::Sell, OrderType::Limit, 105, 8, 2});

    // Act
    bool result = book.cancelOrder(1);

    // Assert
    assert(result);

    assert(book.getTotalOrders() == 1);

    assert(!book.hasOrder(1));

    assert(book.hasOrder(2));

    auto remainingOrder = book.getOrder(2);

    assert(remainingOrder.has_value());

    assert(remainingOrder->quantity == 8);

    std::cout << "✓ PASS: Cancel Existing Order\n";
}


void testCancelNonExistingOrder()
{
    printTestHeader("Cancel Non Existing Order");

    // Arrange
    OrderBook book;

    // Act
    bool result = book.cancelOrder(999);

    // Assert
    assert(!result);

    assert(book.isEmpty());

    std::cout << "✓ PASS: Cancel Non Existing Order\n";
}



void testCancelAfterPartialFill()
{
    printTestHeader("Cancel After Partial Fill");

    // Arrange
    OrderBook book;

    book.addOrder({1, Side::Sell, OrderType::Limit, 100, 8, 1});
    book.addOrder({2, Side::Buy, OrderType::Limit, 100, 3, 2});

    // Act

    bool result = book.cancelOrder(1);

    // Assert

    assert(result);

    assert(book.isEmpty());

    std::cout << "✓ PASS: Cancel After Partial Fill\n";
}


int main()
{
    testCancelExistingOrder();

    testCancelNonExistingOrder();

    testCancelAfterPartialFill();

    std::cout << "\n=====================================\n";
    std::cout << "      ALL CANCELLATION TESTS PASSED\n";
    std::cout << "=====================================\n";

    return 0;
}

