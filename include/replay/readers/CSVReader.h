#pragma once

#include <fstream>
#include <string>

#include "IEventReader.h"

class CSVReader : public IEventReader
{
private:
    std::ifstream file;

public:
    explicit CSVReader(const std::string& filename);

    bool hasNext() override;

    Event next() override;
};