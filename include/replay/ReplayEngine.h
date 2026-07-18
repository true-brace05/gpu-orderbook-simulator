#pragma once

#include "OrderBook.h"
#include "Event.h"
#include <vector>
#include "replay/readers/IEventReader.h"

class ReplayEngine
{
private:
    OrderBook& orderBook;

public:
    explicit ReplayEngine(OrderBook& book);

    void process(const Event& event);



    void replay(IEventReader& reader);

    
};