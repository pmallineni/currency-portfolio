#pragma once
#include <string>


struct Currency
{
    const std::string_view name;
    const char* symbol;
    const std::string_view isoCode;
    const int unitPips;
};


template <const Currency& Instance> 
struct CurrencyTag
{
    static constexpr const Currency& instance = Instance;
};

