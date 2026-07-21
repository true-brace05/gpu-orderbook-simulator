#pragma once

#include <fstream>
#include <string>

#include "replay/readers/IEventReader.h"

class LOBSTERReader : public IEventReader
{
public:
    explicit LOBSTERReader(const std::string& filename);

    bool hasNext()  override;

    Event next() override;

private:
    std::ifstream file;
};