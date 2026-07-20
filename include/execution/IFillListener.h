#pragma once

#include "execution/Fill.h"

class IFillListener
{
public:
    virtual ~IFillListener() = default;

    virtual void onFill(const Fill& fill) = 0;
};