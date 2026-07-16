#include <iostream>
#include "OrderBook.h"

int main()
{
    OrderBook book;
    
    book.setVerbose(true);

    book.addOrder({1, Side::Buy, OrderType::Limit, -100, 5, 1});
book.addOrder({2, Side::Buy, OrderType::Limit, 100, -5, 2});
book.addOrder({1, Side::Buy, OrderType::Limit, 100, 5, 1});
book.addOrder({2, Side::Sell, OrderType::Limit, 105, 5, 2});

book.printBook();

   



   
if(book.cancelOrder(999))
    std::cout << "Wrong\n";
else
    std::cout << "Correct\n";

    std::cout << "\nAfter cancellation:\n";
    book.printBook();

    return 0;
}