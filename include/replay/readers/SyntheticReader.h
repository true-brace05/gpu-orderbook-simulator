#pragma once

#include <vector>

#include "IEventReader.h"

class SyntheticReader : public IEventReader
{
private:
    std::vector<Event> events;

    std::size_t currentIndex = 0;

public:
    explicit SyntheticReader(std::vector<Event> replayEvents);

    bool hasNext() override;

    Event next() override;
};