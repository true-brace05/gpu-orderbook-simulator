#include <iostream>
#include <stdexcept>

#include "replay/readers/CSVReader.h"

int main()
{
    CSVReader reader("tests/data/malformed_event.csv");

    bool rejected = false;

    try
    {
        (void)reader.next();
    }
    catch (const std::runtime_error&)
    {
        rejected = true;
    }

    if (!rejected)
    {
        std::cerr << "Malformed CSV row was accepted\n";
        return 1;
    }

    std::cout << "CSV parser validation test passed!\n";

    return 0;
}
