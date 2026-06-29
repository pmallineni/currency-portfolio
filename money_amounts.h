#pragma once
#include <string>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <cmath> 
#include <iostream>

#include "currency_conversions.h"

using Pip = std::int64_t;
using Years = double;


class  RuntimeMonetaryAmount
{
public: 
    RuntimeMonetaryAmount(Pip p, const Currency& c) : pipAmount(p), currency(&c) {}
    Pip getPipAmount() const noexcept { return pipAmount; }
    const Currency& getCurrency() const noexcept { return *currency; }

    RuntimeMonetaryAmount convertTo(const Currency& targetCurrency) const;

    RuntimeMonetaryAmount in(const Currency& targetCurrency) const { return convertTo(targetCurrency); }
    RuntimeMonetaryAmount as(const Currency& targetCurrency) const { return convertTo(targetCurrency); }

    RuntimeMonetaryAmount& operator+=(RuntimeMonetaryAmount o);
    RuntimeMonetaryAmount& operator-=(RuntimeMonetaryAmount o);
    RuntimeMonetaryAmount& operator*=(Rate rate);
    RuntimeMonetaryAmount& operator*=(std::int64_t scalarVal);

    RuntimeMonetaryAmount operator*(double) const = delete;
    RuntimeMonetaryAmount operator*(float)  const = delete;



private: 
    Pip pipAmount;
    const Currency* currency;
};



template <typename T_CurrencyTag>
class MonetaryAmount
{     
public:

    explicit constexpr MonetaryAmount(Pip p) : pipAmount(p) {}
    Pip getPipAmount() const noexcept { return pipAmount; } 
    constexpr const Currency& getCurrency() const noexcept { return currency; }    
    
    // TODO: Implement a more accurate percentage calculation that factors in the currency's unitPips and rounding rules for the specific currency
    // If making changes, make change to RuntimeMonetaryAmount::percentOf as well
    constexpr MonetaryAmount percentOf(Rate rate) const { 
        __int128_t intermediatePips = static_cast<__int128_t>(pipAmount) * Rate::detail::getRaw(rate);
//        __int128_t roundOffset = Rate::detail::getScale();
//        if (intermediatePips < 0) roundOffset = -roundOffset;
        Pip finalValue = static_cast<Pip>((intermediatePips) / Rate::detail::getScale());
        return MonetaryAmount{finalValue};
    }

    // implicitly converts to RuntimeMonetaryAmount for mixed operations
    operator RuntimeMonetaryAmount() const noexcept { return RuntimeMonetaryAmount{pipAmount, currency};}


    constexpr MonetaryAmount operator+(const MonetaryAmount& m) const { return MonetaryAmount{pipAmount + m.pipAmount}; }      
    constexpr MonetaryAmount operator-(const MonetaryAmount& m) const { return MonetaryAmount{pipAmount - m.pipAmount}; }      
    
    constexpr MonetaryAmount operator*(Rate rate) const { return percentOf(rate); }      
    friend constexpr MonetaryAmount operator*(Rate rate, const MonetaryAmount& m) { return m.percentOf(rate); }

    constexpr MonetaryAmount operator*(std::int64_t scalarVal) const {return MonetaryAmount{pipAmount * scalarVal};}
    friend constexpr MonetaryAmount operator*(std::int64_t scalarVal, const MonetaryAmount& m) {return m * scalarVal;}
    constexpr MonetaryAmount& operator*=(std::int64_t scalarVal) {this->pipAmount *= scalarVal; return *this;}
    constexpr MonetaryAmount& operator*=(Rate rate)
    {
        __int128_t intermediatePips = static_cast<__int128_t>(pipAmount) * Rate::detail::getRaw(rate);
//        __int128_t roundOffset = Rate::detail::getScale();
//        if (intermediatePips < 0) roundOffset = -roundOffset;
        Pip finalValue = static_cast<Pip>((intermediatePips) / Rate::SCALE);
        this->getPipAmount = finalValue;
        return *this;
    }

    // operator* should never be used with type double. Use Rate instead (enforces fixed-point arithmetic)
    constexpr MonetaryAmount operator*(double) const = delete;

    // operator* should never be used with type float. Use Rate instead (enforces fixed-point arithmetic)
    constexpr MonetaryAmount operator*(float) const = delete;


    constexpr MonetaryAmount& operator+=(const MonetaryAmount& m) { pipAmount += m.pipAmount; return *this; }      
    constexpr MonetaryAmount& operator-=(const MonetaryAmount& m) { pipAmount -= m.pipAmount; return *this; }      

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
    // temporary function; going to create a View interface which should deal with output
    friend std::ostream& operator<<(std::ostream& out, const MonetaryAmount& m) {
        Pip dollars {m.pipAmount / currency.unitPips };
        Pip cents {(m.pipAmount % currency.unitPips) / 100 };
        return out << currency.symbol << dollars << '.' << cents;
    }

private:     
    Pip pipAmount;
    static constexpr const Currency& currency = T_CurrencyTag::instance;
};


inline void validateCurrency(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount rhs)
{
    if(&lhs.getCurrency() != &rhs.getCurrency()) throw std::runtime_error("Currency mismatch. Use explicit conversion");
}

inline RuntimeMonetaryAmount& RuntimeMonetaryAmount::operator+=(const RuntimeMonetaryAmount o)
{
    validateCurrency(*this, o);
    this->pipAmount += o.pipAmount;
    return *this;
}

inline RuntimeMonetaryAmount& RuntimeMonetaryAmount::operator-=(const RuntimeMonetaryAmount o)
{
    validateCurrency(*this, o);
    this->pipAmount -= o.pipAmount;
    return *this;
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

inline RuntimeMonetaryAmount getPercentOf(const RuntimeMonetaryAmount& lhs, Rate rate) 
{
    __int128_t intermediatePips = static_cast<__int128_t>(lhs.getPipAmount()) * Rate::detail::getRaw(rate);
//    __int128_t roundOffset = Rate::detail::getScale();
//    if (intermediatePips < 0) roundOffset = -roundOffset;
    Pip finalValue = static_cast<Pip>((intermediatePips) / Rate::detail::getScale());
    return RuntimeMonetaryAmount{finalValue, lhs.getCurrency()};
}

inline RuntimeMonetaryAmount operator*(const RuntimeMonetaryAmount& lhs, Rate rhs)
{
    return getPercentOf(lhs, rhs);
}

inline RuntimeMonetaryAmount operator*(Rate lhs, const RuntimeMonetaryAmount& rhs)
{
    return getPercentOf(rhs, lhs);
}
inline RuntimeMonetaryAmount& RuntimeMonetaryAmount::operator*=(Rate rate)
{
    __int128_t intermediatePips = static_cast<__int128_t>(this->pipAmount) * Rate::detail::getRaw(rate);
//    __int128_t roundOffset = Rate::detail::getScale();
//    if (intermediatePips < 0) roundOffset = -roundOffset;
    Pip finalValue = static_cast<Pip>((intermediatePips) / Rate::detail::getScale());
    this->pipAmount = finalValue;
    return *this;
}

inline RuntimeMonetaryAmount& RuntimeMonetaryAmount::operator*=(std::int64_t scalarVal)
{
    this->pipAmount *= scalarVal;
    return *this;
}

inline RuntimeMonetaryAmount operator*(const RuntimeMonetaryAmount& lhs, std::int64_t scalarVal)
{
    return RuntimeMonetaryAmount(lhs.getPipAmount() * scalarVal, lhs.getCurrency());
}

inline RuntimeMonetaryAmount operator*(std::int64_t scalarVal, const RuntimeMonetaryAmount& rhs)
{
    return RuntimeMonetaryAmount(rhs.getPipAmount() * scalarVal, rhs.getCurrency());
}
inline RuntimeMonetaryAmount RuntimeMonetaryAmount::convertTo(const Currency& targetCurrency) const
{
        if (currency == &targetCurrency) return *this;
        const CurrencyConverterService& service {CurrencyConverterService::instance()};
        Rate rate {service.getRate(*currency, targetCurrency)};
//        __int128_t amount = (static_cast<__int128_t>(targetCurrency.unitPips) * getPipAmount() * Rate::detail::getRaw(rate)) / getCurrency().unitPips;
        __int128_t amount = getPipAmount();     
        amount  *= Rate::detail::getRaw(rate);
        amount  *= targetCurrency.unitPips;
        amount /= (Rate::detail::getScale() * currency->unitPips);
        return RuntimeMonetaryAmount{static_cast<Pip>(amount), targetCurrency};
}
inline bool operator==(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    validateCurrency(lhs, rhs);
    return lhs.getPipAmount() == rhs.getPipAmount();
}

inline bool operator!=(const RuntimeMonetaryAmount& lhs, const RuntimeMonetaryAmount& rhs)
{
    validateCurrency(lhs, rhs);
    return lhs.getPipAmount() != rhs.getPipAmount();
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

// Mixed operations boilerplate
// Because RuntimeMonetaryAmount and MonetaryAmount are fundamentally different
// inheritance to an abstract class is very messy (and not worth it)
// because of this, a lot more boilerplate is required though

template <typename T_CurrencyTag>
inline RuntimeMonetaryAmount operator+(const RuntimeMonetaryAmount& lhs, const MonetaryAmount<T_CurrencyTag>& rhs)
{
    assert(&lhs.getCurrency() == &T_CurrencyTag::instance);
    return lhs + static_cast<RuntimeMonetaryAmount>(rhs);
}

template <typename T_CurrencyTag>
inline RuntimeMonetaryAmount operator+(const MonetaryAmount<T_CurrencyTag>& lhs, const RuntimeMonetaryAmount& rhs)
{
    assert(&rhs.getCurrency() == &T_CurrencyTag::instance);
    return static_cast<RuntimeMonetaryAmount>(lhs) + rhs;
}

template <typename T_CurrencyTag>
inline RuntimeMonetaryAmount operator-(const RuntimeMonetaryAmount& lhs, const MonetaryAmount<T_CurrencyTag>& rhs)
{
    assert(&lhs.getCurrency() == &T_CurrencyTag::instance);
    return lhs - static_cast<RuntimeMonetaryAmount>(rhs);
}

template <typename T_CurrencyTag>
inline RuntimeMonetaryAmount operator-(const MonetaryAmount<T_CurrencyTag>& lhs, const RuntimeMonetaryAmount& rhs)
{
    assert(&rhs.getCurrency() == &T_CurrencyTag::instance);
    return static_cast<RuntimeMonetaryAmount>(lhs) - rhs;
}

template <typename T_CurrencyTag>
inline bool operator==(const RuntimeMonetaryAmount& lhs, const MonetaryAmount<T_CurrencyTag>& rhs)
{
    return lhs == static_cast<RuntimeMonetaryAmount>(rhs);
}

template <typename T_CurrencyTag>
inline bool operator==(const MonetaryAmount<T_CurrencyTag>& lhs, const RuntimeMonetaryAmount& rhs)
{
    return static_cast<RuntimeMonetaryAmount>(lhs) == rhs;
}

template <typename T_CurrencyTag>
inline bool operator!=(const RuntimeMonetaryAmount& lhs, const MonetaryAmount<T_CurrencyTag>& rhs)
{
    return lhs != static_cast<RuntimeMonetaryAmount>(rhs);
}

template <typename T_CurrencyTag>
inline bool operator!=(const MonetaryAmount<T_CurrencyTag>& lhs, const RuntimeMonetaryAmount& rhs)
{
    return static_cast<RuntimeMonetaryAmount>(lhs) != rhs;
}


template <typename T_CurrencyTag>
inline bool operator<(const RuntimeMonetaryAmount& lhs, const MonetaryAmount<T_CurrencyTag>& rhs)
{
    return lhs < static_cast<RuntimeMonetaryAmount>(rhs);
}

template <typename T_CurrencyTag>
inline bool operator<(const MonetaryAmount<T_CurrencyTag>& lhs, const RuntimeMonetaryAmount& rhs)
{
    return static_cast<RuntimeMonetaryAmount>(lhs) < rhs;
}

template <typename T_CurrencyTag>
inline bool operator>(const RuntimeMonetaryAmount& lhs, const MonetaryAmount<T_CurrencyTag>& rhs)
{
    return lhs > static_cast<RuntimeMonetaryAmount>(rhs);
}

template <typename T_CurrencyTag>
inline bool operator>(const MonetaryAmount<T_CurrencyTag>& lhs, const RuntimeMonetaryAmount& rhs)
{
    return static_cast<RuntimeMonetaryAmount>(lhs) > rhs;
}

template <typename T_CurrencyTag>
inline bool operator<=(const RuntimeMonetaryAmount& lhs, const MonetaryAmount<T_CurrencyTag>& rhs)
{
    return lhs <= static_cast<RuntimeMonetaryAmount>(rhs);
}

template <typename T_CurrencyTag>
inline bool operator<=(const MonetaryAmount<T_CurrencyTag>& lhs, const RuntimeMonetaryAmount& rhs)
{
    return static_cast<RuntimeMonetaryAmount>(lhs) <= rhs;
}

template <typename T_CurrencyTag>
inline bool operator>=(const RuntimeMonetaryAmount& lhs, const MonetaryAmount<T_CurrencyTag>& rhs)
{
    return lhs >= static_cast<RuntimeMonetaryAmount>(rhs);
}

template <typename T_CurrencyTag>
inline bool operator>=(const MonetaryAmount<T_CurrencyTag>& lhs, const RuntimeMonetaryAmount& rhs)
{
    return static_cast<RuntimeMonetaryAmount>(lhs) >= rhs;
}