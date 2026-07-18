#include <cassert>
#include <iostream>

#include "OrderBook.h"
#include "replay/ReplayEngine.h"
#include "replay/readers/CSVReader.h"

int main()
{
    OrderBook book;

    ReplayEngine replay(book);

    CSVReader reader("../data/sample.csv");

    replay.replay(reader);

    assert(book.getTotalOrders() == 3);

    std::cout << "CSV Replay Test Passed!\n";

    return 0;
}