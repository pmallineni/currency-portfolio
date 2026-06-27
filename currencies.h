#pragma once

#include "money_amounts.h"

constexpr Currency US_DOLLAR = {"US Dollar", "$", "USD", 10000 };
constexpr Currency EURO = {"Euro", "€", "EUR", 10000 };
constexpr Currency BRITISH_POUND = {"British Pound", "£", "GBP", 10000 };
constexpr Currency JAPANESE_YEN = {"Japanese Yen", "¥", "JPY", 100 };

DECLARE_CURRENCY_LITERAL(USDAmount, US_DOLLAR, USD)
DECLARE_CURRENCY_LITERAL(EURAmount, EURO, EUR)
DECLARE_CURRENCY_LITERAL(GBPAmount, BRITISH_POUND, GBP)
DECLARE_CURRENCY_LITERAL(JPYAmount, JAPANESE_YEN, JPY)

using USD = CurrencyTag<US_DOLLAR>;
using EUR = CurrencyTag<EURO>;
using GBP = CurrencyTag<BRITISH_POUND>;
using JPY = CurrencyTag<JAPANESE_YEN>;

namespace CurrencyInit
{
    inline PresetCurrencyConverter presetConverter;
    inline void setupConversions()
    {
        presetConverter.setRate(EURO, US_DOLLAR,      1.1340);
        presetConverter.setRate(EURO, JAPANESE_YEN,   183.35);
        presetConverter.setRate(EURO, BRITISH_POUND,  0.86165);
        presetConverter.setRate(US_DOLLAR, JAPANESE_YEN, 161.58);
        presetConverter.setRate(BRITISH_POUND, JAPANESE_YEN, 212.80); // EUR/GBP * EUR/JPY inverse

        CurrencyConverterService::instance().setConverter(std::make_unique<PresetCurrencyConverter>(std::move(presetConverter)));
    }

}
