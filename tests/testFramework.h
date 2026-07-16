#pragma once

#include <iostream>

inline void printTestHeader(const std::string& name)
{
    std::cout << "\n==============================\n";
    std::cout << "Running: " << name << '\n';
    std::cout << "==============================\n";
}