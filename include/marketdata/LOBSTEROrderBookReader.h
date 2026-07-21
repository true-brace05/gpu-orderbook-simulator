#pragma once

#include <fstream>
#include <string>

#include "marketdata/OrderBookSnapshot.h"

class LOBSTEROrderBookReader
{
private:
    std::ifstream file;

public:
    explicit LOBSTEROrderBookReader(
        const std::string& filename);

    bool hasNext();

    OrderBookSnapshot next();
};