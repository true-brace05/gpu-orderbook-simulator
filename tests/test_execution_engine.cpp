#include <iostream>

#include "execution/ExecutionEngine.h"

class CountingFillListener : public IFillListener
{
public:
    void onFill(const Fill&) override
    {
        ++notifications;
    }

    int notifications = 0;
};

int main()
{
    ExecutionEngine engine;
    CountingFillListener listener;

    engine.addFillListener(&listener);
    engine.onTrade({1, 2, 100.0, 5, 1});

    if (listener.notifications != 1)
    {
        std::cerr << "Expected one fill notification, received "
                  << listener.notifications << '\n';
        return 1;
    }

    std::cout << "Execution engine listener test passed!\n";

    return 0;
}
