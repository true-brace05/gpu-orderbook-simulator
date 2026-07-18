#pragma once

#include <cstdint>

#include "Order.h"
#include "Types.h"

struct Event
{
    uint64_t timestamp;

    EventType type;

    Order order;

    int orderId = -1;
};