#pragma once

#include "types.h"

constexpr Currency US_DOLLAR = {"US Dollar", "$", "USD", 10000 };
constexpr Currency EURO = {"Euro", "€", "EUR", 10000 };
constexpr Currency BRITISH_POUND = {"British Pound", "£", "GBP", 10000 };
constexpr Currency JAPANESE_YEN = {"Japanese Yen", "¥", "JPY", 100 };

DECLARE_CURRENCY_LITERAL(USDAmount, US_DOLLAR, USD)
DECLARE_CURRENCY_LITERAL(EURAmount, EURO, EUR)
DECLARE_CURRENCY_LITERAL(GBPAmount, BRITISH_POUND, GBP)
DECLARE_CURRENCY_LITERAL(JPYAmount, JAPANESE_YEN, JPY)