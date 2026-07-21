#include <iostream>

#include "marketdata/LOBSTERReader.h"

std::string eventTypeToString(EventType type)
{
    switch (type)
    {
        case EventType::Add:
            return "Add";

        case EventType::Cancel:
            return "Cancel";

        case EventType::Delete:
            return "Delete";

        case EventType::ExecuteVisible:
            return "ExecuteVisible";

        case EventType::ExecuteHidden:
            return "ExecuteHidden";

        case EventType::TradingHalt:
            return "TradingHalt";
    }

    return "Unknown";
}

std::string sideToString(Side side)
{
    switch (side)
    {
        case Side::Buy:
            return "Buy";

        case Side::Sell:
            return "Sell";
    }

    return "Unknown";
}

int main()
{
    LOBSTERReader reader(
    "../data/lobster/LOBSTER_SampleFile_AAPL_2012-06-21_10/"
    "AAPL_2012-06-21_34200000_57600000_message_10.csv");

    for (int i = 0; i < 10 && reader.hasNext(); ++i)
    {
        Event event = reader.next();

        std::cout << "Timestamp : "
                  << event.timestamp << '\n';

        std::cout << "Event     : "
                  << eventTypeToString(event.type) << '\n';

        std::cout << "Order ID  : "
                  << event.order.id << '\n';

        std::cout << "Quantity  : "
                  << event.order.quantity << '\n';

        std::cout << "Price     : "
                  << event.order.price << '\n';

        std::cout << "Side      : "
                  << sideToString(event.order.side) << '\n';

        std::cout << "-------------------------------------\n";
    }

    return 0;
}