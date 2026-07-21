#pragma once

#include "OrderBook.h"
#include "Event.h"
#include <vector>
#include "replay/readers/IEventReader.h"
#include "strategy/IStrategy.h"
#include "replay/IEventListener.h"
#include "replay/ReplayStatistics.h"

class ReplayEngine
{
private:
    OrderBook& orderBook;
    IStrategy* strategy = nullptr;

     void process(const Event& event);

     std::vector<IEventListener*> eventListeners;

     ReplayStatistics statistics;

public:
    explicit ReplayEngine(OrderBook& book);

   

void addEventListener(IEventListener* listener);

    void replay(IEventReader& reader);

void replay(
    IEventReader& reader,
    std::size_t maxEvents);

const ReplayStatistics& getStatistics() const;
    void setStrategy(IStrategy* newStrategy);
    
};