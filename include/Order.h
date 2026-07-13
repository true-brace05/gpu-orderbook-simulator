#pragma once 
#include "Types.h"

struct Order{
    int id ; 
    Side side ;
    double  price ;
    int quantity ;
    long long timestamp ;

};