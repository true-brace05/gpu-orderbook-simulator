#pragma once

#include "OrderBook.h"
#include "Event.h"
#include <vector>
#include "replay/readers/IEventReader.h"
#include "strategy/IStrategy.h"
#include "replay/IEventListener.h"


class ReplayEngine
{
private:
    OrderBook& orderBook;
    IStrategy* strategy = nullptr;

     void process(const Event& event);

     std::vector<IEventListener*> eventListeners;

public:
    explicit ReplayEngine(OrderBook& book);

   

void addEventListener(IEventListener* listener);

    void replay(IEventReader& reader);



    void setStrategy(IStrategy* newStrategy);
    
};