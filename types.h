#pragma once
#include <string>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <cmath> 

#include "currency_types.h"

#define DECLARE_CURRENCY_LITERAL(TAG, INSTANCE, SUFFIX)             \
    using TAG = MonetaryAmount<CurrencyTag<INSTANCE>>;              \
    constexpr TAG operator"" _ ## SUFFIX(unsigned long long p)      \
    {                                                               \
        if (p > static_cast<unsigned long long>(                    \
                std::numeric_limits<Pip>::max()))                   \
            throw std::range_error("Literal exceeds Pip range");    \
        return TAG(static_cast<Pip>(p));                            \
    }


using Pip = std::int64_t;
using Years = double;


template <typename T_CurrencyTag>
class MonetaryAmount
{     
public:

    explicit constexpr MonetaryAmount(Pip p) : pipAmount(p) {}
    Pip getPipAmount() const noexcept { return pipAmount; } 
    constexpr Currency& getCurrency() const noexcept { return currency; }    
    
    // TODO: Implement a more accurate percentage calculation that factors in the currency's unitPips and rounding rules for the specific currency
    // If making changes, make change to RuntimeMonetaryAmount::percentOf
    constexpr MonetaryAmount percentOf(double percentage) const { 
        Pip newPipAmount = static_cast<Pip>(std::floor(pipAmount * percentage));
        return {newPipAmount};
    }

    constexpr MonetaryAmount operator+(const MonetaryAmount& m) const { return {pipAmount + m.pipAmount}; }      
    constexpr MonetaryAmount operator-(const MonetaryAmount& m) const { return {pipAmount - m.pipAmount}; }      
    constexpr MonetaryAmount operator*(const MonetaryAmount& m) const { return {pipAmount * m.pipAmount}; }     
    
    constexpr MonetaryAmount operator*(double percentage) const { return percentOf(percentage); }      
    friend constexpr MonetaryAmount operator*(double percentage, const MonetaryAmount& m) { return m.percentOf(percentage); }

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
    static constexpr Currency& currency = T_CurrencyTag::instance;
};


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
/*
    constexpr MonetaryAmount operator+(const MonetaryAmount& m) const { return {pipAmount + m.pipAmount}; }      
    constexpr MonetaryAmount operator-(const MonetaryAmount& m) const { return {pipAmount - m.pipAmount}; }      
    constexpr MonetaryAmount operator*(const MonetaryAmount& m) const { return {pipAmount * m.pipAmount}; }     
    
    constexpr MonetaryAmount operator*(double percentage) const { return getPercentOf(percentage); }      
    friend constexpr MonetaryAmount operator*(double percentage, const MonetaryAmount& m) { return m.getPercentOf(percentage); }

    constexpr MonetaryAmount& operator+=(const MonetaryAmount& m) { pipAmount += m.pipAmount; return *this; }      
    constexpr MonetaryAmount& operator-=(const MonetaryAmount& m) { pipAmount -= m.pipAmount; return *this; }      
    constexpr MonetaryAmount& operator*=(const MonetaryAmount& m) { pipAmount *= m.pipAmount; return *this; }      

*/


/*
    // TODO: Implement a more accurate percentage calculation that factors in the currency's unitPips and rounding rules for the specific currency
    constexpr MonetaryAmount getPercentOf(double percentage) const { 
        Pip newPipAmount = static_cast<Pip>(std::floor(pipAmount * percentage));
        return {newPipAmount};
    }
*/

inline void validateCurrency(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount rhs)
{
    if(&lhs.getCurrency() != &rhs.getCurrency()) throw std::runtime_error("Currency mismatch. Use explicit conversion");
}

inline RuntimeMonetaryAmount operator+(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    validateCurrency(lhs, rhs);
    return {lhs.getPipAmount() + rhs.getPipAmount(), lhs.getCurrency()};
}

inline RuntimeMonetaryAmount operator-(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    validateCurrency(lhs, rhs);
    return {lhs.getPipAmount() - rhs.getPipAmount(), lhs.getCurrency()};
}

inline RuntimeMonetaryAmount operator*(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    validateCurrency(lhs, rhs);
    return {lhs.getPipAmount() * rhs.getPipAmount(), lhs.getCurrency()};
}

inline RuntimeMonetaryAmount getPercentOf(const RuntimeMonetaryAmount& lhs, double percentage) 
{
    Pip newPipAmount = static_cast<Pip>(std::floor(lhs.getPipAmount() * percentage));
    return {newPipAmount, lhs.getCurrency()};
}

inline RuntimeMonetaryAmount operator*(const RuntimeMonetaryAmount& lhs, double rhs)
{
    return getPercentOf(lhs, rhs);
}

inline RuntimeMonetaryAmount operator*(double lhs, const RuntimeMonetaryAmount& rhs)
{
    return getPercentOf(rhs, lhs);
}


inline bool operator==(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    validateCurrency(lhs, rhs);
    return lhs.getPipAmount() == rhs.getPipAmount();
}
inline bool operator<(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    validateCurrency(lhs, rhs);
    return lhs.getPipAmount() < rhs.getPipAmount();
}
inline bool operator>(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    validateCurrency(lhs, rhs);
    return rhs < lhs;
}
inline bool operator<=(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    validateCurrency(lhs, rhs);
    return !(rhs < lhs);
}
inline bool operator>=(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    validateCurrency(lhs, rhs);
    return !(lhs < rhs);
}