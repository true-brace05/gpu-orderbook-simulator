#include <iostream>

#include "OrderBook.h"
#include "marketdata/LOBSTERReader.h"
#include "marketdata/LOBSTEROrderBookReader.h"
#include "marketdata/LOBSTERValidator.h"
#include "replay/ReplayEngine.h"

int main()
{
    OrderBook book;
    book.setVerbose(false);

    ReplayEngine replay(book);

    LOBSTERReader messageReader(
        "../data/lobster/LOBSTER_SampleFile_AAPL_2012-06-21_10/"
        "AAPL_2012-06-21_34200000_57600000_message_10.csv");

    LOBSTEROrderBookReader orderbookReader(
        "../data/lobster/LOBSTER_SampleFile_AAPL_2012-06-21_10/"
        "AAPL_2012-06-21_34200000_57600000_orderbook_10.csv");

    // Replay exactly one historical event
    replay.replay(messageReader, 1000);

    // Our reconstructed book
    

    OrderBookSnapshot ours = book.getSnapshot();

std::cout << "Best Bid "
          << ours.bids[0].price
          << " "
          << ours.bids[0].quantity
          << '\n';

std::cout << "Best Ask "
          << ours.asks[0].price
          << " "
          << ours.asks[0].quantity
          << '\n';

    // Official LOBSTER snapshot
   // Official LOBSTER snapshot after 1000 messages
OrderBookSnapshot expected;

for (int i = 0; i < 1000; ++i)
{
    expected = orderbookReader.next();
}

    std::cout << "=============================\n";
    std::cout << "Snapshot Sizes\n";
    std::cout << "=============================\n";

    std::cout << "Our bids      : " << ours.bids.size() << '\n';
    std::cout << "Expected bids : " << expected.bids.size() << '\n';

    std::cout << "Our asks      : " << ours.asks.size() << '\n';
    std::cout << "Expected asks : " << expected.asks.size() << '\n';

    std::cout << '\n';

    if (LOBSTERValidator::validate(ours, expected))
    {
        std::cout << "Validation Passed!\n";
    }
    else
    {
        std::cout << "Validation Failed!\n";

        LOBSTERValidator::printDifferences(
            ours,
            expected);
    }

    return 0;
}