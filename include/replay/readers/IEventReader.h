#pragma once

#include "replay/Event.h"

class IEventReader
{
public:

    virtual bool hasNext() = 0;

    virtual Event next() = 0;

    virtual ~IEventReader() = default;
};