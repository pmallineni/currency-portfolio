#include "portfolio.h"

bool Portfolio::addInstrument(std::unique_ptr<FinancialInstrument> instrument)
{
    if (!instrument) return false;
    instruments_.push_back(std::move(instrument));
    return true;
}

void Portfolio::removeInstrument(FinancialInstrument& instrument)
{
    // TODO implement individual IDs for each FinancialInstrument rather than comparing pointers, as this is not a reliable way to identify unique instruments
    auto removeCondition = [&instrument](const std::unique_ptr<FinancialInstrument>& ptr) { return ptr.get() == &instrument; };
    
    auto newEnd = std::remove_if(instruments_.begin(), instruments_.end(), removeCondition);
    instruments_.erase(newEnd, instruments_.end());
    // C++20 syntax for the above would be:
    // std::erase_if(instruments, removeCondition);
}

void Portfolio::checkInvariants() const 
{
    for (const auto& instrument : instruments_)
    {
        assert(instrument && "Invariant violated: stored nullptr instrument in Portfolio.");
    }
}
