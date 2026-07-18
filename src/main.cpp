#include <iostream>
#include "OrderBook.h"

int main()
{
   
    OrderBook book;

    book.setVerbose(true);

    // Resting sell order
    book.addOrder({
        1,
        Side::Sell,
        OrderType::Limit,
        100,
        5,
        1
    });

    std::cout << "Before Market Order\n";
    book.printBook();

    // Incoming market buy
    book.addOrder({
        2,
        Side::Buy,
        OrderType::Market,
        0,
        3,
        2
    });

    std::cout << "\nAfter Market Order\n";
    book.printBook();

}