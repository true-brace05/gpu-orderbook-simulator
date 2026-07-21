#include <iostream>

#include "marketdata/LOBSTEROrderBookReader.h"

int main()
{
    LOBSTEROrderBookReader reader(
        "../data/lobster/LOBSTER_SampleFile_AAPL_2012-06-21_10/"
        "AAPL_2012-06-21_34200000_57600000_orderbook_10.csv");

    OrderBookSnapshot snapshot = reader.next();

    std::cout << "Best Ask : "
              << snapshot.asks[0].price
              << " "
              << snapshot.asks[0].quantity
              << '\n';

    std::cout << "Best Bid : "
              << snapshot.bids[0].price
              << " "
              << snapshot.bids[0].quantity
              << '\n';

    return 0;
}