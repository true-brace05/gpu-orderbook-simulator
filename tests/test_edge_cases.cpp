#include <cassert>
#include <iostream>

#include "OrderBook.h"
#include "testFramework.h"



void testInvalidPrice()
{
    printTestHeader("Invalid Price");

    // Arrange
    OrderBook book;

    // Act
    book.addOrder({1, Side::Buy, OrderType::Limit, -100, 5, 1});

    // Assert
    assert(book.isEmpty());

    std::cout << "✓ PASS: Invalid Price\n";
}




void testInvalidQuantity()
{
    printTestHeader("Invalid Quantity");

    // Arrange
    OrderBook book;

    // Act
    book.addOrder({1, Side::Buy, OrderType::Limit, 100, -5, 1});

    // Assert
    assert(book.isEmpty());

    std::cout << "✓ PASS: Invalid Quantity\n";
}


void testCancelEmptyBook()
{
    printTestHeader("Cancel Empty Book");

    // Arrange
    OrderBook book;

    // Act
    bool result = book.cancelOrder(1);

    // Assert
    assert(!result);

    assert(book.isEmpty());

    std::cout << "✓ PASS: Cancel Empty Book\n";
}





void testDuplicateOrderId()
{
    printTestHeader("Duplicate Order ID");

    // Arrange
    OrderBook book;

    // Act
    book.addOrder({1, Side::Buy, OrderType::Limit, 100, 5, 1});
    book.addOrder({1, Side::Sell, OrderType::Limit, 105, 3, 2});

    // Assert

    assert(book.getTotalOrders() == 1);

    std::cout << "✓ PASS: Duplicate Order ID\n";
}

void testMarketOrderZeroPrice()
{
    printTestHeader("Market Order Zero Price");

    OrderBook book;
    book.setVerbose(false);

    book.addOrder({
        1,
        Side::Buy,
        OrderType::Market,
        0,
        10,
        1
    });

    // Market order should not be rejected because of price = 0.
    // Since the opposite book is empty, nothing should rest.

    assert(book.isEmpty());
    assert(book.getTotalOrders() == 0);

    std::cout << "✓ PASS: Market Order Zero Price\n";
}



int main()
{
    testInvalidPrice();

    testInvalidQuantity();

    testCancelEmptyBook();

    testDuplicateOrderId();

    testMarketOrderZeroPrice();

    std::cout << "\n=====================================\n";
    std::cout << "      ALL EDGE CASE TESTS PASSED\n";
    std::cout << "=====================================\n";

    return 0;
}





