#pragma once

#include "OrderBook.h"
#include "Event.h"
#include <vector>
#include "replay/readers/IEventReader.h"
#include "strategy/IStrategy.h"

class ReplayEngine
{
private:
    OrderBook& orderBook;
    IStrategy* strategy = nullptr;

     void process(const Event& event);

public:
    explicit ReplayEngine(OrderBook& book);

   



    void replay(IEventReader& reader);



    void setStrategy(IStrategy* newStrategy);
    
};