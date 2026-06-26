#pragma once
#include <string>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <cmath> 

using Pip = std::int64_t;
using Years = double;

enum class IsoCode
{
    USD, 
    EUR,
    GBP,
    JPY
};

constexpr std::string_view getIsoCodeName(IsoCode code)
{
    switch (code)
    {
        case(IsoCode::USD): return "USD";
        case(IsoCode::EUR): return "EUR";
        case(IsoCode::GBP): return "GBP";
        case(IsoCode::JPY): return "JPY";
    }
    static_assert("getIsoCodeName not implemented for this type");
    return "???";
}

struct Currency
{
    const std::string_view name;
    const char* symbol;
    const IsoCode isoCode;
    const int unitPips;
};


constexpr Currency US_DOLLAR = {"US Dollar", "$", IsoCode::USD, 10000 };
constexpr Currency EURO = {"Euro", "€", IsoCode::EUR, 10000 };
constexpr Currency BRITISH_POUND = {"British Pound", "£", IsoCode::GBP, 10000 };
constexpr Currency JAPANESE_YEN = {"Japanese Yen", "¥", IsoCode::JPY, 10000 };




template <const Currency& Instance> 
struct CurrencyTag
{
    static constexpr const Currency& instance = Instance;
};

template <typename T_CurrencyTag>
class MonetaryAmount
{     
public:

    explicit constexpr MonetaryAmount(Pip p) : pipAmount(p) {}
    Pip getPipAmount() const noexcept { return pipAmount; } 
    constexpr const Currency& getCurrency() const noexcept { return currency; }    
    
    // TODO: Implement a more accurate percentage calculation that factors in the currency's unitPips and rounding rules for the specific currency
    constexpr MonetaryAmount getPercentOf(double percentage) const { 
        Pip newPipAmount = static_cast<Pip>(std::floor(pipAmount * percentage));
        return {newPipAmount};
    }

    constexpr MonetaryAmount operator+(const MonetaryAmount& m) const { return {pipAmount + m.pipAmount}; }      
    constexpr MonetaryAmount operator-(const MonetaryAmount& m) const { return {pipAmount - m.pipAmount}; }      
    constexpr MonetaryAmount operator*(const MonetaryAmount& m) const { return {pipAmount * m.pipAmount}; }     
    
    constexpr MonetaryAmount operator*(double percentage) const { return getPercentOf(percentage); }      
    friend constexpr MonetaryAmount operator*(double percentage, const MonetaryAmount& m) { return m.getPercentOf(percentage); }

    constexpr MonetaryAmount& operator+=(const MonetaryAmount& m) { pipAmount += m.pipAmount; return *this; }      
    constexpr MonetaryAmount& operator-=(const MonetaryAmount& m) { pipAmount -= m.pipAmount; return *this; }      
    constexpr MonetaryAmount& operator*=(const MonetaryAmount& m) { pipAmount *= m.pipAmount; return *this; }      


    friend bool operator==(const MonetaryAmount& lhs, const MonetaryAmount& rhs) noexcept {
        return lhs.pipAmount == rhs.pipAmount;
    }
    friend bool operator<(const MonetaryAmount& lhs, const MonetaryAmount& rhs) noexcept {
        return lhs.pipAmount < rhs.pipAmount;
    }

    friend bool operator!=(const MonetaryAmount& lhs, const MonetaryAmount& rhs) noexcept { return !(lhs == rhs); }
    friend bool operator> (const MonetaryAmount& lhs, const MonetaryAmount& rhs) noexcept { return rhs < lhs; }
    friend bool operator<=(const MonetaryAmount& lhs, const MonetaryAmount& rhs) noexcept { return !(rhs < lhs); }
    friend bool operator>=(const MonetaryAmount& lhs, const MonetaryAmount& rhs) noexcept { return !(lhs < rhs); }

    // TODO: Implement a more accurate output operator that takes into account the currency and formats the output accordingly
    friend std::ostream& operator<<(std::ostream& out, const MonetaryAmount& m) {
        Pip dollars {m.pipAmount / currency.unitPips };
        Pip cents {(m.pipAmount % currency.unitPips) / 100 };
        return out << currency.symbol << dollars << '.' << cents;
    }

private:     
    Pip pipAmount;
    static constexpr const Currency& currency = T_CurrencyTag::instance;
};

namespace CurrencyLiterals
{
    using USDAmount = MonetaryAmount<CurrencyTag<US_DOLLAR>>;
    using EURAmount = MonetaryAmount<CurrencyTag<EURO>>;
    using GBPAmt = MonetaryAmount<CurrencyTag<BRITISH_POUND>>;
    using JPYAmt = MonetaryAmount<CurrencyTag<JAPANESE_YEN>>;

    constexpr MonetaryAmount<CurrencyTag<US_DOLLAR>> operator"" _USD(unsigned long long p)
    {
        static_assert(sizeof(p) >= sizeof(Pip), "Platform Mismatch: Pip type is larger than unsigned long long");
        if (p > static_cast<unsigned long long>(std::numeric_limits<Pip>::max())) throw std::range_error("Literal value exceeds maximum value for Pip type");
        return MonetaryAmount<CurrencyTag<US_DOLLAR>>(static_cast<Pip>(p)); 
    }

    constexpr MonetaryAmount<CurrencyTag<EURO>> operator"" _EUR(unsigned long long p)
    {
        static_assert(sizeof(p) >= sizeof(Pip), "Platform Mismatch: Pip type is larger than unsigned long long");
        if (p > static_cast<unsigned long long>(std::numeric_limits<Pip>::max())) throw std::range_error("Literal value exceeds maximum value for Pip type");
        return MonetaryAmount<CurrencyTag<EURO>>(static_cast<Pip>(p)); 
    }

    constexpr MonetaryAmount<CurrencyTag<BRITISH_POUND>> operator"" _GBP(unsigned long long p)
    {
        static_assert(sizeof(p) >= sizeof(Pip), "Platform Mismatch: Pip type is larger than unsigned long long");
        if (p > static_cast<unsigned long long>(std::numeric_limits<Pip>::max())) throw std::range_error("Literal value exceeds maximum value for Pip type");
        return MonetaryAmount<CurrencyTag<BRITISH_POUND>>(static_cast<Pip>(p)); 
    }

    constexpr MonetaryAmount<CurrencyTag<JAPANESE_YEN>> operator"" _JPY(unsigned long long p)
    {
        static_assert(sizeof(p) >= sizeof(Pip), "Platform Mismatch: Pip type is larger than unsigned long long");
        if (p > static_cast<unsigned long long>(std::numeric_limits<Pip>::max())) throw std::range_error("Literal value exceeds maximum value for Pip type");
        return MonetaryAmount<CurrencyTag<JAPANESE_YEN>>(static_cast<Pip>(p)); 
    }

    constexpr std::string_view getCurrencyName(const USDAmount& m) {return m.getCurrency().name;}
    constexpr std::string_view getCurrencyName(const EURAmount& m) {return m.getCurrency().name;}
    constexpr std::string_view getCurrencyName(const GBPAmt& m) {return m.getCurrency().name;}
    constexpr std::string_view getCurrencyName(const JPYAmt& m) {return m.getCurrency().name;}
}



using namespace CurrencyLiterals;
using Years = double;


class  RuntimeMonetaryAmount
{
public: 
    
    RuntimeMonetaryAmount(Pip p, const Currency& c) : pipAmount(p), currency(c) {}
    Pip getPipAmount() const noexcept { return pipAmount; }
    const Currency& getCurrency() const noexcept { return currency; }

    // TODO implement currency conversion
    RuntimeMonetaryAmount convertedTo(const Currency& targetCurrency) const
    {
        if (&currency == &targetCurrency) return *this;
        else throw std::runtime_error("Currency conversion not implemented yet");
    }

private: 

    Pip pipAmount;
    const Currency& currency;
};

inline bool operator==(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    if (&lhs.getCurrency() != &rhs.getCurrency()) throw std::runtime_error("Currency mismatch. Use explicit conversion");
    return lhs.getPipAmount() == rhs.getPipAmount();
}
inline bool operator<(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    if (&lhs.getCurrency() != &rhs.getCurrency()) throw std::runtime_error("Currency mismatch. Use explicit conversion");
    return lhs.getPipAmount() < rhs.getPipAmount();
}
inline bool operator>(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    if (&lhs.getCurrency() != &rhs.getCurrency()) throw std::runtime_error("Currency mismatch. Use explicit conversion");
    return rhs < lhs;
}
inline bool operator<=(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    if (&lhs.getCurrency() != &rhs.getCurrency()) throw std::runtime_error("Currency mismatch. Use explicit conversion");
    return !(rhs < lhs);
}
inline bool operator>=(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    if (&lhs.getCurrency() != &rhs.getCurrency()) throw std::runtime_error("Currency mismatch. Use explicit conversion");
    return !(lhs < rhs);
}