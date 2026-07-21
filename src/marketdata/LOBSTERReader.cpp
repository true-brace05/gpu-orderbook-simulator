#include "marketdata/LOBSTERReader.h"

#include <stdexcept>
#include <sstream>
#include <vector>
namespace
{

std::vector<std::string> split(
    const std::string& line)
{
    std::vector<std::string> fields;

    std::stringstream stream(line);

    std::string token;

    while (std::getline(stream, token, ','))
    {
        fields.push_back(token);
    }

    return fields;
}

uint64_t parseTimestamp(
    const std::string& value)
{
    double seconds = std::stod(value);

    return static_cast<uint64_t>(
        seconds * 1'000'000'000.0);
}


Side parseSide(
    const std::string& value)
{
    if (value == "1")
    {
        return Side::Buy;
    }

    if (value == "-1")
    {
        return Side::Sell;
    }

    throw std::runtime_error(
        "Invalid LOBSTER side: " + value);
}

EventType parseEventType(
    const std::string& value)
{
    const int event = std::stoi(value);

    switch (event)
    {
        case 1:
            return EventType::Add;

        case 2:
            return EventType::Cancel;

        case 3:
            return EventType::Delete;

        case 4:
            return EventType::ExecuteVisible;

        case 5:
            return EventType::ExecuteHidden;

        case 7:
            return EventType::TradingHalt;

        default:
            throw std::runtime_error(
                "Unknown LOBSTER event type: " + value);
    }
}
double parsePrice(
    const std::string& value)
{
    constexpr double PRICE_SCALE = 10000.0;

    return std::stod(value) / PRICE_SCALE;
}

} // namespace
LOBSTERReader::LOBSTERReader(const std::string& filename)
    : file(filename)
{
    if (!file.is_open())
    {
        throw std::runtime_error(
            "Failed to open LOBSTER file: " + filename);
    }
}

bool LOBSTERReader::hasNext() 
{
    return file.good();
}

Event LOBSTERReader::next()
{
    std::string line;

    std::getline(file, line);

    if (line.empty())
    {
        throw std::runtime_error(
            "Unexpected empty line in LOBSTER file.");
    }

    auto fields = split(line);

    if (fields.size() != 6)
    {
        throw std::runtime_error(
    "Failed to open LOBSTER file: " );
    }

    Event event;

    event.timestamp =
    parseTimestamp(fields[0]);

event.type =
    parseEventType(fields[1]);

event.orderId =
    std::stoi(fields[2]);

switch (event.type)
{
    case EventType::Add:
    {
        event.order.id = event.orderId;
        event.order.quantity = std::stoi(fields[3]);
        event.order.price = parsePrice(fields[4]);
        event.order.side = parseSide(fields[5]);
        event.order.type = OrderType::Limit;
        event.order.timestamp = event.timestamp;
        break;
    }

    case EventType::Cancel:
case EventType::Delete:
case EventType::ExecuteVisible:
case EventType::ExecuteHidden:
{
    event.order.id = event.orderId;
    event.order.quantity = std::stoi(fields[3]);
    event.order.price = parsePrice(fields[4]);
    event.order.side = parseSide(fields[5]);
    event.order.timestamp = event.timestamp;
    break;
}
}
    return event;
}