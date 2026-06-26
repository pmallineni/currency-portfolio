#include <memory>
#include <vector>
#include <algorithm>
#include <cassert>

#include "types.h"
#include "instruments.h"

class Portfolio
{
public: 
    Portfolio(std::string portfolioName, std::vector<std::unique_ptr<FinancialInstrument>> instruments) : portfolioName_(portfolioName), instruments_(std::move(instruments)) {}
    Portfolio (std::string portfolioName) : portfolioName_(portfolioName) {}
    Portfolio() : portfolioName_(defaultPortfolioName) {}

    Portfolio(Portfolio&&) noexcept = default;
    Portfolio& operator=(Portfolio&&) noexcept = default;

    Portfolio(const Portfolio&) = delete;
    Portfolio& operator=(const Portfolio&) = delete;

    const auto& getInstruments() const {return instruments_;} 
    std::string_view getPortfolioName() const noexcept {return portfolioName_;}
    std::int64_t getPortfolioID() const noexcept {return portfolioID_;}

    template <typename TargetCurrencyTag, bool ConvertCurrency = true>
    MonetaryAmount<TargetCurrencyTag> getTotalValue() const
    {
        return getTotalAccumulation<TargetCurrencyTag, ConvertCurrency>(InstrumentFunctions::valueFunction);
    }

    template <typename TargetCurrencyTag, bool ConvertCurrency = true>
    MonetaryAmount<TargetCurrencyTag> getTotalReturn() const
    {
        return getTotalAccumulation<TargetCurrencyTag, ConvertCurrency>(InstrumentFunctions::returnFunction);
    }

    template <typename TargetCurrencyTag, bool ConvertCurrency = true>
    MonetaryAmount<TargetCurrencyTag> getTotalReturn(Years holdingPeriod) const
    {
        auto returnFunction = InstrumentFunctions::holdingPeriodReturnFunction(holdingPeriod);
        return getTotalAccumulation<TargetCurrencyTag, ConvertCurrency>(returnFunction);
    }

    bool addInstrument(std::unique_ptr<FinancialInstrument> instrument);

    void removeInstrument(FinancialInstrument& instrument);


private:
    std::string portfolioName_;
    std::vector<std::unique_ptr<FinancialInstrument>> instruments_;
    std::int64_t portfolioID_ {++portfolioIDNum};
    static inline std::int16_t portfolioIDNum {0};
    static constexpr std::string_view defaultPortfolioName {"Portfolio"};


    template <typename TargetCurrencyTag, bool ConvertCurrency = true,  typename AccumulationFunc>
    MonetaryAmount<TargetCurrencyTag> getTotalAccumulation(AccumulationFunc accumulationFunction) const
    {
        Pip totalPips {0}; 

        for (const auto& instrument : instruments_)
        {
            
            RuntimeMonetaryAmount instrumentValue = accumulationFunction(instrument.get());
            if (&instrumentValue.currency == &TargetCurrencyTag::instance)
            {
                totalPips += instrumentValue.getPipAmount();
            }
            else
            {
                if constexpr (ConvertCurrency) static_assert(!sizeof(AccumulationFunc), "Currency conversion not implemented yet");
                else throw std::runtime_error("Currency mismatch and conversion not allowed");
            }
        }


        return {totalPips};
    }

    void checkInvariants() const;
};



