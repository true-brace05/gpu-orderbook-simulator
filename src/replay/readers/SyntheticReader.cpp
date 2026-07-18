#include "replay/readers/SyntheticReader.h"

SyntheticReader::SyntheticReader(std::vector<Event> replayEvents)
    : events(std::move(replayEvents))
{
}

bool SyntheticReader::hasNext()
{
    return currentIndex < events.size();
}

Event SyntheticReader::next()
{
    return events[currentIndex++];
}