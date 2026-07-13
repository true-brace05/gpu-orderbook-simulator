#include <iostream>
#include "OrderBook.h"

int main()
{
    OrderBook book;

    Order order1{
        1,
        Side::Buy,
        100.0,
        10,
        1
    };

    Order order2{
        2,
        Side::Buy,
        101.0,
        5,
        2
    };

    Order order3{
        3,
        Side::Sell,
        102.0,
        8,
        3
    };

    book.addOrder(order1);
    book.addOrder(order2);
    book.addOrder(order3);

    book.printBook();

    return 0;
}