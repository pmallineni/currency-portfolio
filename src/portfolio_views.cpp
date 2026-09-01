#include "portfolio_views.h"
#include "portfolio.h"

namespace PortfolioViews
{
    std::vector<const FinancialInstrument*> getByValue(const Portfolio& portfolio, bool ascending)
    {
        auto valueFunction = InstrumentFunctions::valueFunction;
        return details::getSortedInstrumentbyMoneyFunc(portfolio, valueFunction, ascending);
    }

    std::vector<const FinancialInstrument*> getByReturn(const Portfolio& portfolio, bool ascending)
    {
        auto returnFunction = InstrumentFunctions::returnFunction;
        return details::getSortedInstrumentbyMoneyFunc(portfolio, returnFunction, ascending);
    }

    std::vector<const FinancialInstrument*> getByReturn(const Portfolio& portfolio, Years holdingPeriod, bool ascending)
    {
        auto returnFunction = InstrumentFunctions::holdingPeriodReturnFunction(holdingPeriod);
        return details::getSortedInstrumentbyMoneyFunc(portfolio, returnFunction, ascending);
    }

    std::vector<const FinancialInstrument*> getByID(const Portfolio& portfolio, bool ascending)
    {
        auto idFunction = InstrumentFunctions::idFunction;
        return details::getSortedInstrumentByFunc(portfolio, idFunction, ascending);
    }

    std::vector<const FinancialInstrument*> getByName(const Portfolio& portfolio, bool lettersAtoZ)
    {
        auto nameFunction = InstrumentFunctions::nameFunction;
        return details::getSortedInstrumentByFunc(portfolio, nameFunction, lettersAtoZ);
    }
}