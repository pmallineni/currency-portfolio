#pragma once

#include "money_amounts.h"

constexpr Currency US_DOLLAR = {"US Dollar", "$", "USD", 1000000 };
constexpr Currency EURO = {"Euro", "€", "EUR", 1000000 };
constexpr Currency BRITISH_POUND = {"British Pound", "£", "GBP", 1000000 };
constexpr Currency JAPANESE_YEN = {"Japanese Yen", "¥", "JPY", 10000 };


using USD = CurrencyTag<US_DOLLAR>;
using EUR = CurrencyTag<EURO>;
using GBP = CurrencyTag<BRITISH_POUND>;
using JPY = CurrencyTag<JAPANESE_YEN>;


// USD, EUR, GBP all in cents (lowest possible integer unit)
using USDAmount = MonetaryAmount<USD>;
constexpr USDAmount operator"" _USD(unsigned long long p)
{
    int unitPips {USD::instance.unitPips};
    if (p > static_cast<unsigned long long>( std::numeric_limits<Pip>::max() / unitPips)) throw std::range_error("Literal exceeds Pip range");
    return USDAmount(static_cast<Pip>(p * unitPips / 100)); 
}

using EURAmount = MonetaryAmount<EUR>;
constexpr EURAmount operator"" _EUR(unsigned long long p)
{
    int unitPips {EUR::instance.unitPips};
    if (p > static_cast<unsigned long long>( std::numeric_limits<Pip>::max() / unitPips)) throw std::range_error("Literal exceeds Pip range");
    return EURAmount(static_cast<Pip>(p * unitPips / 100)); 
}

using GBPAmount = MonetaryAmount<GBP>;
constexpr GBPAmount operator"" _GBP(unsigned long long p)
{
    int unitPips {GBP::instance.unitPips};
    if (p > static_cast<unsigned long long>( std::numeric_limits<Pip>::max() / unitPips)) throw std::range_error("Literal exceeds Pip range");
    return GBPAmount(static_cast<Pip>(p * unitPips / 100)); 
}

using JPYAmount = MonetaryAmount<JPY>;
constexpr JPYAmount operator"" _JPY(unsigned long long p)
{
    int unitPips {JPY::instance.unitPips};
    if (p > static_cast<unsigned long long>( std::numeric_limits<Pip>::max() / unitPips)) throw std::range_error("Literal exceeds Pip range");
    return JPYAmount(static_cast<Pip>(p * unitPips)); 
}



namespace CurrencyInit
{
    inline void setupConversions()
    {
        PresetCurrencyConverter presetConverter;
        presetConverter.setRate(EURO, US_DOLLAR,             Rate::from_parts(113910255));
        presetConverter.setRate(EURO, JAPANESE_YEN,          Rate::from_parts(18424420624));
        presetConverter.setRate(EURO, BRITISH_POUND,         Rate::from_parts(86270025));
        presetConverter.setRate(US_DOLLAR, JAPANESE_YEN,     Rate::from_parts(16179515984   ));
        presetConverter.setRate(BRITISH_POUND, JAPANESE_YEN, Rate::from_parts(21353948904));
        presetConverter.setRate(US_DOLLAR, BRITISH_POUND,    Rate::from_parts(75736071));
        
        CurrencyConverterService::instance().setConverter(std::make_unique<PresetCurrencyConverter>(std::move(presetConverter)));
    }

}
