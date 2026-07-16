#pragma once 
#include "Types.h"

struct Order{
    int id;

Side side;

OrderType type;

double price;

int quantity;

uint64_t timestamp;

};