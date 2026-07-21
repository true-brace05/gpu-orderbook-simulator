#include <iostream>

#include "OrderBook.h"
#include "marketdata/LOBSTERReader.h"
#include "replay/ReplayEngine.h"

int main()
{
    OrderBook book;
book.setVerbose(false);

ReplayEngine replay(book);
    LOBSTERReader reader(
        "../data/lobster/LOBSTER_SampleFile_AAPL_2012-06-21_10/"
        "AAPL_2012-06-21_34200000_57600000_message_10.csv");

    replay.replay(reader, 1000);
    const ReplayStatistics& stats =
    replay.getStatistics();

    std::cout << "\n";
std::cout << "=====================================\n";
std::cout << "      Replay Statistics\n";
std::cout << "=====================================\n";

std::cout << "Processed Events : "
          << stats.processedEvents << '\n';

std::cout << "Add Events       : "
          << stats.addEvents << '\n';

std::cout << "Cancel Events    : "
          << stats.cancelEvents << '\n';

std::cout << "Delete Events    : "
          << stats.deleteEvents << '\n';

std::cout << "Visible Executes : "
          << stats.visibleExecutions << '\n';

std::cout << "Hidden Executes  : "
          << stats.hiddenExecutions << '\n';

std::cout << "Trading Halts    : "
          << stats.tradingHalts << '\n';

std::cout << "Orders In Book   : "
          << book.getTotalOrders() << '\n';

std::cout << "Replay Time (ms) : "
          << stats.replayTimeMs
          << '\n';

std::cout << "Events / Second  : "
          << stats.eventsPerSecond
          << '\n';
//book.printBook();

return 0;

    
}