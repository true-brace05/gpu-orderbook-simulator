#include "cuda/hello_kernel.h"

#include <iostream>
#include <exception>

int main()
{
    try
    {
        std::cout << "==============================\n";
        std::cout << "Running: GPU Hello Kernel Test\n";
        std::cout << "==============================\n";

        launchHelloKernel();

        std::cout << "✓ PASS: CUDA Kernel Launch & Execution\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "✗ FAIL: " << ex.what() << '\n';
        return 1;
    }
}
