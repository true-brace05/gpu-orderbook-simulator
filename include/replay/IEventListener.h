#pragma once

#include "replay/Event.h"

class IEventListener
{
public:
    virtual ~IEventListener() = default;

    virtual void onEvent(const Event& event) = 0;
};