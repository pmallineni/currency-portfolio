#pragma once
#include <vector>
#include <algorithm>

#include "instruments.h"
#include "portfolio.h"

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

        template <typename ReturnsRuntimeMonetaryAmount, bool ConvertCurrency = true>
        std::vector<const FinancialInstrument*> getSortedInstrumentbyMoneyFunc(
            const Portfolio& portfolio, 
            ReturnsRuntimeMonetaryAmount instrumentFunc, 
            bool ascending = true
        )
        {
            using ReturnType = std::invoke_result_t<ReturnsRuntimeMonetaryAmount, const FinancialInstrument*>;
            static_assert(
                std::is_same_v<ReturnType, RuntimeMonetaryAmount>,
                "instrumentFunc must return a RuntimeMonetaryAmount. "
                "Did you mean to use getSortedInstrumentFunc instead?"
            );
            
            if constexpr (ConvertCurrency)
            {
                if (ascending)
                    return getSortedInstrumentView(portfolio,
                        [instrumentFunc](const FinancialInstrument* a, const FinancialInstrument* b)
                        { 
                            RuntimeMonetaryAmount lhs {instrumentFunc(a)};
                            RuntimeMonetaryAmount rhs {instrumentFunc(b)};
                            return lhs < rhs.as(lhs.getCurrency());
                        }
                    );
                else 
                    return getSortedInstrumentView(portfolio,
                        [instrumentFunc](const FinancialInstrument* a, const FinancialInstrument* b)
                        { 
                            RuntimeMonetaryAmount lhs {instrumentFunc(a)};
                            RuntimeMonetaryAmount rhs {instrumentFunc(b)};
                            return lhs > rhs.as(lhs.getCurrency());
                        }
                    );
                    

            }
            else return getSortedInstrumentByFunc<ReturnsRuntimeMonetaryAmount>(portfolio, instrumentFunc, ascending);
        }

    }

    std::vector<const FinancialInstrument*> getByValue(const Portfolio& portfolio, bool ascending = true);
    std::vector<const FinancialInstrument*> getByReturn(const Portfolio& portfolio, bool ascending = true);
    std::vector<const FinancialInstrument*> getByReturn(const Portfolio& portfolio, Years holdingPeriod, bool ascending= true);
    std::vector<const FinancialInstrument*> getByID(const Portfolio& portfolio, bool ascending = true);
    std::vector<const FinancialInstrument*> getByName(const Portfolio& portfolio, bool lettersAtoZ = true);


}