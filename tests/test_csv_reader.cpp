#include <cassert>
#include <iostream>

#include "replay/readers/CSVReader.h"

int main()
{
    CSVReader reader("../data/sample.csv");

    Event first = reader.next();

    assert(first.timestamp == 1);
    assert(first.type == EventType::Add);

    assert(first.order.id == 1);
    assert(first.order.side == Side::Buy);
    assert(first.order.type == OrderType::Limit);
    assert(first.order.price == 100);
    assert(first.order.quantity == 5);

    std::cout << "First event parsed successfully!\n";

    Event second = reader.next();

    assert(second.timestamp == 2);
    assert(second.order.id == 2);
    assert(second.order.side == Side::Sell);

    std::cout << "Second event parsed successfully!\n";

    Event third = reader.next();

    assert(third.timestamp == 3);
    assert(third.order.id == 3);

    std::cout << "Third event parsed successfully!\n";

    std::cout << "\nCSV Reader Test Passed!\n";

    return 0;
}