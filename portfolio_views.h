#pragma once
#include <vector>
#include <algorithm>

#include "types.h"

class Portfolio;
class FinancialInstrument;

namespace PortfolioViews
{
    namespace details
    {
        template <typename Compare>
        std::vector<const FinancialInstrument*> getSortedInstrumentView(const Portfolio& portfolio, Compare comp)
        {
            const auto& instruments {portfolio.getInstruments()};
            std::vector<const FinancialInstrument*> view; 
            view.reserve(instruments.size());

            for (const auto& instrument : instruments)
            {
                if (instrument)
                    view.push_back(instrument.get());
            }

            std::sort(view.begin(), view.end(), comp);
            return view;
        }

        template <typename ReturnsComparable>
        std::vector<const FinancialInstrument*> getSortedInstrumentByFunc(
            const Portfolio& portfolio,
            ReturnsComparable instrumentFunc,
            bool ascending = true)
        {
            if (ascending)
                return getSortedInstrumentView(portfolio, 
                    [instrumentFunc](const FinancialInstrument* a, const FinancialInstrument* b)
                    { return instrumentFunc(a) < instrumentFunc(b);}
                );
            else
                return getSortedInstrumentView(portfolio, 
                    [instrumentFunc](const FinancialInstrument* a, const FinancialInstrument* b)
                    { return instrumentFunc(a) > instrumentFunc(b);}
                );
        }

    }

    std::vector<const FinancialInstrument*> getByValue(const Portfolio& portfolio, bool ascending = true);
    std::vector<const FinancialInstrument*> getByReturn(const Portfolio& portfolio, bool ascending = true);
    std::vector<const FinancialInstrument*> getByReturn(const Portfolio& portfolio, Years holdingPeriod, bool ascending= true);
    std::vector<const FinancialInstrument*> getByID(const Portfolio& portfolio, bool ascending = true);
    std::vector<const FinancialInstrument*> getByName(const Portfolio& portfolio, bool lettersAtoZ = true);
    

}