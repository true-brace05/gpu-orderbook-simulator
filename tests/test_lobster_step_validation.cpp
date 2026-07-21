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

    for (int event = 1; event <= 20; ++event)
    {
        // Replay exactly one message
        replay.replay(messageReader, 1);

        // Our reconstructed snapshot
        OrderBookSnapshot ours = book.getSnapshot();

        // Official LOBSTER snapshot
        OrderBookSnapshot expected = orderbookReader.next();

        std::cout << "\n=====================================\n";
        std::cout << "EVENT " << event << '\n';
        std::cout << "=====================================\n";

        std::cout << "\nOURS\n";

        for (const auto& bid : ours.bids)
        {
            std::cout << "Bid "
                      << bid.price
                      << " @ "
                      << bid.quantity
                      << '\n';
        }

        for (const auto& ask : ours.asks)
        {
            std::cout << "Ask "
                      << ask.price
                      << " @ "
                      << ask.quantity
                      << '\n';
        }

        std::cout << "\nEXPECTED\n";

        for (const auto& bid : expected.bids)
        {
            std::cout << "Bid "
                      << bid.price
                      << " @ "
                      << bid.quantity
                      << '\n';
        }

        for (const auto& ask : expected.asks)
        {
            std::cout << "Ask "
                      << ask.price
                      << " @ "
                      << ask.quantity
                      << '\n';
        }

        if (!LOBSTERValidator::validate(ours, expected))
        {
            std::cout << "\n=====================================\n";
            std::cout << "First mismatch at event "
                      << event
                      << '\n';
            std::cout << "=====================================\n\n";

            LOBSTERValidator::printDifferences(
                ours,
                expected);

            return 0;
        }
    }

    std::cout << "\nPerfect! First 20 events matched.\n";

    return 0;
}