#include <cassert>
#include <iostream>

#include "OrderBook.h"
#include "replay/ReplayEngine.h"
#include "replay/readers/CSVReader.h"

int main()
{
    OrderBook book;
    ReplayEngine replay(book);
    CSVReader reader("tests/data/sample_cancel.csv");

    replay.replay(reader);

    assert(book.isEmpty());
    assert(!book.hasOrder(1));

    std::cout << "CSV cancel replay test passed!\n";

    return 0;
}
