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
    book.addOrder({1, Side::Buy, -100, 5, 1});

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
    book.addOrder({1, Side::Buy, 100, -5, 1});

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
    book.addOrder({1, Side::Buy, 100, 5, 1});
    book.addOrder({1, Side::Sell, 105, 3, 2});

    // Assert

    assert(book.getTotalOrders() == 1);

    std::cout << "✓ PASS: Duplicate Order ID\n";
}





int main()
{
    testInvalidPrice();

    testInvalidQuantity();

    testCancelEmptyBook();

    testDuplicateOrderId();

    std::cout << "\n=====================================\n";
    std::cout << "      ALL EDGE CASE TESTS PASSED\n";
    std::cout << "=====================================\n";

    return 0;
}





