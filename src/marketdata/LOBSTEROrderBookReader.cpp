#include "marketdata/LOBSTEROrderBookReader.h"

#include <sstream>
#include <stdexcept>
#include <vector>

LOBSTEROrderBookReader::LOBSTEROrderBookReader(
    const std::string& filename)
    : file(filename)
{
    if (!file.is_open())
    {
        throw std::runtime_error(
            "Failed to open orderbook file.");
    }
}

bool LOBSTEROrderBookReader::hasNext()
{
    return file.good();
}

OrderBookSnapshot LOBSTEROrderBookReader::next()
{
    std::string line;

    std::getline(file, line);

    OrderBookSnapshot snapshot;

    if (line.empty())
    {
        return snapshot;
    }

    std::stringstream stream(line);

    std::string token;

    std::vector<std::string> fields;

    while (std::getline(stream, token, ','))
    {
        fields.push_back(token);
    }

    constexpr double PRICE_SCALE = 10000.0;

    for (std::size_t i = 0; i + 3 < fields.size(); i += 4)
    {
        PriceLevel ask;
        ask.price = std::stod(fields[i]) / PRICE_SCALE;
        ask.quantity = std::stoi(fields[i + 1]);

        PriceLevel bid;
        bid.price = std::stod(fields[i + 2]) / PRICE_SCALE;
        bid.quantity = std::stoi(fields[i + 3]);

        snapshot.asks.push_back(ask);
        snapshot.bids.push_back(bid);
    }

    return snapshot;
}