#pragma once 
#include "Types.h"
#include <cstdint>

struct Order{
    int id;

Side side;

OrderType type;

double price;

int quantity;

uint64_t timestamp;

// Version 1.0 Iceberg simplification: submitted visible quantity must equal
// displayQuantity. This is not a general exchange rule.
int displayQuantity = 0;

int reserveQuantity = 0;

};
