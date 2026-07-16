#include <cassert>
#include <iostream>

#include "OrderBook.h"
#include "testFramework.h"

void testExactMatch()
{
    printTestHeader("Exact Match");

    OrderBook book;

    book.addOrder({1, Side::Sell, 100, 5, 1});
    book.addOrder({2, Side::Buy, 100, 5, 2});

    assert(book.isEmpty());

    std::cout << "✓ PASS: Exact Match\n";
}


void testPartialFill()
{   
    printTestHeader("Partial Fill");
    // Arrange
    OrderBook book;

    // Act
    book.addOrder({1, Side::Sell, 100, 8, 1});
    book.addOrder({2, Side::Buy, 100, 3, 2});

    // Assert
    assert(!book.isEmpty());

assert(book.getTotalOrders() == 1);

auto remainingOrder = book.getOrder(1);

assert(remainingOrder.has_value());

assert(remainingOrder->quantity == 5);

assert(remainingOrder->price == 100);

assert(remainingOrder->side == Side::Sell);

assert(!book.hasOrder(2));

std::cout << "✓ PASS: Partial Fill\n";
}

void testFIFO()
{
    printTestHeader("FIFO");

    // Arrange
    OrderBook book;

    book.addOrder({1, Side::Sell, 100, 5, 1});
    book.addOrder({2, Side::Sell, 100, 3, 2});

    // Act
    book.addOrder({3, Side::Buy, 100, 6, 3});

    // Assert
    assert(book.getTotalOrders() == 1);

auto remainingOrder = book.getOrder(2);

assert(remainingOrder.has_value());

assert(remainingOrder->quantity == 2);

assert(remainingOrder->price == 100);

assert(remainingOrder->timestamp == 2);

assert(!book.hasOrder(1));

assert(!book.hasOrder(3));

std::cout << "✓ PASS: FIFO\n";}

void testMultiPriceLevel()
{
    printTestHeader("Multi Price Level Matching");

    // Arrange
    OrderBook book;

    book.addOrder({1, Side::Sell, 100, 2, 1});
    book.addOrder({2, Side::Sell, 101, 4, 2});
    book.addOrder({3, Side::Sell, 102, 6, 3});

    // Act
    book.addOrder({4, Side::Buy, 102, 10, 4});

    // Assert
    assert(book.getTotalOrders() == 1);

    assert(!book.hasOrder(1));
    assert(!book.hasOrder(2));
   auto remainingOrder = book.getOrder(3);

assert(remainingOrder.has_value());

assert(remainingOrder->quantity == 2);

assert(remainingOrder->price == 102);

assert(remainingOrder->timestamp == 3);
    assert(!book.hasOrder(4));

    std::cout << "✓ PASS: Multi Price Level Matching\n";
}

void testNoMatch()
{
    printTestHeader("No Match");

    // Arrange
    OrderBook book;

    // Act
    book.addOrder({1, Side::Buy, 100, 5, 1});
    book.addOrder({2, Side::Sell, 105, 5, 2});

    // Assert
    assert(book.getTotalOrders() == 2);

    assert(book.hasOrder(1));
    assert(book.hasOrder(2));

    std::cout << "✓ PASS: No Match\n";
}

int main()
{
    testExactMatch();
    testPartialFill();
    testFIFO();
    testMultiPriceLevel();
    testNoMatch();

    std::cout << "\n=====================================\n";
    std::cout << "      ALL MATCHING TESTS PASSED\n";
    std::cout << "=====================================\n";


    return 0;
}