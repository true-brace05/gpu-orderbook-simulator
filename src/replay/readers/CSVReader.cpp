#include "replay/readers/CSVReader.h"

#include <stdexcept>
#include <sstream>
#include <vector>

#include <iostream>
#include <filesystem>

CSVReader::CSVReader(const std::string& filename)
{
    

    file.open(filename);

    if (!file.is_open())
    {
        throw std::runtime_error(
            "Failed to open CSV file: " + filename
        );
    }

    std::string header;
    std::getline(file, header);
}

bool CSVReader::hasNext()
{
    return file.peek() != EOF;
}

Event CSVReader::next()
{
    std::string line;

    std::getline(file, line);

    if (line.empty())
    {
        return {};
    }

    std::stringstream ss(line);

    std::string token;

    std::vector<std::string> fields;

    while (std::getline(ss, token, ','))
    {
        fields.push_back(token);
    }

   Event event;

// Timestamp
event.timestamp = std::stoull(fields[0]);

// Event Type
if (fields[1] == "Add")
{
    event.type = EventType::Add;
}
else if (fields[1] == "Cancel")
{
    event.type = EventType::Cancel;
}
else if (fields[1] == "Modify")
{
    event.type = EventType::Modify;
}
else
{
    throw std::runtime_error("Unknown EventType");
}

// Side
if (fields[2] == "Buy")
{
    event.order.side = Side::Buy;
}
else if (fields[2] == "Sell")
{
    event.order.side = Side::Sell;
}
else
{
    throw std::runtime_error("Unknown Side");
}

// Order Type
if (fields[3] == "Limit")
{
    event.order.type = OrderType::Limit;
}
else if (fields[3] == "Market")
{
    event.order.type = OrderType::Market;
}
else
{
    throw std::runtime_error("Unknown OrderType");
}

// Price
event.order.price = std::stod(fields[4]);

// Quantity
event.order.quantity = std::stoi(fields[5]);

// Order ID
event.order.id = std::stoi(fields[6]);

event.order.timestamp = event.timestamp;

return event;
}