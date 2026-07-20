#pragma once

#include <cstdint>

#include "Types.h"

struct Fill
{
    Side side;
    double price;
    int quantity;
    std::int64_t timestamp;
};