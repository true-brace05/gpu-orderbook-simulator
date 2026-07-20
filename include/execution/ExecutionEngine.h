#include "execution/ITradeListener.h"
#include "execution/IFillListener.h"
#include <vector>
#include <optional>

class ExecutionEngine : public ITradeListener
{
private:
    std::vector<IFillListener*> fillListeners;
public:
    std::optional<Fill> processTrade(
        const Trade& trade,
        Side strategySide);

    void addFillListener(IFillListener* listener);

void onTrade(const Trade& trade) override;
};