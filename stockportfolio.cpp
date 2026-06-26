#include <iostream>
#include <cstdint>
#include <sstream>
#include <cmath>
#include <memory>
#include <vector>
#include <functional>
#include <algorithm>
#include <type_traits>

using Pip = std::int64_t;

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

// using CRTP to create base class
// In C++20 would use Concepts instead (much simpler)

template <typename T, typename = std::void_t<>>
struct has_getCurrency : std::false_type {};

template <typename T>
struct has_getCurrency<T, std::void_t<decltype(std::declval<const T>().getCurrency())>>
    : std::integral_constant<bool,
        std::is_same<
            decltype(std::declval<const T>().getCurrency()),
            const Currency&
        >::value>
{};

template <typename T, typename = std::void_t<>>
struct has_getPipAmount : std::false_type {};

template <typename T>
struct has_getPipAmount<T, std::void_t<decltype(std::declval<const T>().getPipAmount())>>
    : std::integral_constant<bool,
        std::is_same<
            decltype(std::declval<const T>().getPipAmount()),
            Pip
        >::value>
{};

template <typename T>
constexpr bool has_getCurrency_v = has_getCurrency<T>::value;

template <typename T>
constexpr bool has_getPipAmount_v = has_getPipAmount<T>::value;




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




class FinancialInstrument
{
    public:
    virtual RuntimeMonetaryAmount getValue() const  = 0;
    virtual RuntimeMonetaryAmount getReturn() const = 0;
    virtual RuntimeMonetaryAmount getReturn(Years holdingPeriod) const = 0;
    virtual std::int64_t getID() const noexcept = 0;
    virtual std::string_view getName() const noexcept = 0;
    virtual void addYearsToHoldingPeriod(Years years) = 0;
    virtual ~FinancialInstrument() = default; 
};

template <typename T_CurrencyTag>
class TypedFinancialInstrument : public FinancialInstrument
{
    public: 
    virtual MonetaryAmount<T_CurrencyTag> getSpecificValue() const = 0;
    virtual MonetaryAmount<T_CurrencyTag> getSpecificReturn() const = 0;
    virtual MonetaryAmount<T_CurrencyTag> getSpecificReturn(Years holdingPeriod) const = 0;

    RuntimeMonetaryAmount getValue() const override
    {
        MonetaryAmount<T_CurrencyTag> specificValue = getSpecificValue();
        return {specificValue.getPipAmount(), specificValue.getCurrency()};
    }

    RuntimeMonetaryAmount getReturn() const override
    {
        MonetaryAmount<T_CurrencyTag> specificReturn = getSpecificReturn();
        return {specificReturn.getPipAmount(), specificReturn.getCurrency()};
    }

    RuntimeMonetaryAmount getReturn(Years holdingPeriod) const override
    {
        MonetaryAmount<T_CurrencyTag> specificReturn = getSpecificReturn(holdingPeriod);
        return {specificReturn.getPipAmount(), specificReturn.getCurrency()};
    }



};

template <typename T_CurrencyTag>
class Stock : public TypedFinancialInstrument<T_CurrencyTag>
{
    public: 
    Stock(MonetaryAmount<T_CurrencyTag> priceBought, std::string tickerSymbol, std::string name="Stock", Years holdingPeriod = 0) :
    priceBought_(priceBought), value_(priceBought), tickerSymbol_(tickerSymbol), name_(name), holdingPeriod_(holdingPeriod) {}



    MonetaryAmount<T_CurrencyTag> getValue() const override {return value_;}
    std::string_view getName() const noexcept override {return name_;}
    std::int64_t getID() const noexcept override {return stockID_;}
    std::string_view getTickerSymbol() const noexcept {return tickerSymbol_;}

    MonetaryAmount<T_CurrencyTag> getReturn() const override
    {
        return MonetaryAmount(value_.getPipAmount() - priceBought_.getPipAmount());
    }
    
    // TODO: implement a more accurate return calculation that implements some sort of arbitrary distribution (normal distribution by default) taking account inputted holding period
    MonetaryAmount<T_CurrencyTag> getReturn(Years holdingPeriod) const override
    {
        if (holdingPeriod < 0) throw (std::range_error("holdingPeriod for a financial return cannot be less than 0"));
        if (holdingPeriod > holdingPeriod_) throw (std::range_error("holdingPeriod for a financial return cannot be greater than the holding period of the stock"));

        return MonetaryAmount(value_.getPipAmount() - priceBought_.getPipAmount());
    }
    
    void setValue(MonetaryAmount<T_CurrencyTag> newValue) {value_ = newValue;}
    void addYearsToHoldingPeriod(Years years) {holdingPeriod_ += years;}
    private: 
    MonetaryAmount<T_CurrencyTag> priceBought_;
    MonetaryAmount<T_CurrencyTag> value_;
    std::string_view tickerSymbol_;
    std::string_view name_;
    Years holdingPeriod_;
    
    inline static std::int64_t stockIDNum {0}; 
    const std::int64_t stockID_{++stockIDNum};
    
};

template <typename T_CurrencyTag>
class Bond : public TypedFinancialInstrument<T_CurrencyTag>
{
    public:
    Bond(MonetaryAmount<T_CurrencyTag> principal, double couponRate, Years payoutPeriod = 0.5, Years holdingPeriod = 0) :
    principal_(principal), value_(principal), couponRate_(couponRate), payoutPeriod_(payoutPeriod), holdingPeriod_(holdingPeriod), name_(defaultName){}
    
    Bond(MonetaryAmount<T_CurrencyTag> principal, double couponRate, std::string name, Years payoutPeriod = 0.5, Years holdingPeriod = 0) :
    principal_(principal), value_(principal), couponRate_(couponRate), payoutPeriod_(payoutPeriod), holdingPeriod_(holdingPeriod), name_(name) {}

    MonetaryAmount<T_CurrencyTag> getValue() const override {return value_;} // maybe modifiable if I include buy and sell
    MonetaryAmount<T_CurrencyTag> getReturn(Years holdingPeriod) const
    {
        if (holdingPeriod < 0) throw (std::range_error("holdingPeriod for a financial return cannot be less than 0"));

        std::int64_t numInterestPeriods = static_cast<std::int64_t>(std::floor(holdingPeriod / payoutPeriod_));
        return MonetaryAmount<T_CurrencyTag>(principal_ * numInterestPeriods * couponRate_); // simple Interest
    }
    MonetaryAmount<T_CurrencyTag> getReturn() const override
    {
        return getReturn(holdingPeriod_);
    }

    std::int64_t getID() const noexcept override {return bondID_;}
    std::string_view getName() const noexcept override {return name_;}

    void addYearsToHoldingPeriod(Years years) {holdingPeriod_ += years;}


    private:
    MonetaryAmount<T_CurrencyTag> principal_; 
    MonetaryAmount<T_CurrencyTag> value_;
    double couponRate_;
    Years payoutPeriod_;
    Years holdingPeriod_;
    std::string_view name_;

    inline static std::int64_t bondIDNum {0};
    const std::int64_t bondID_{++bondIDNum};
    static constexpr std::string_view defaultName {"Bond"};


};

template <typename T_CurrencyTag>
class Cash : public TypedFinancialInstrument<T_CurrencyTag>
{
    public: 
    Cash(MonetaryAmount<T_CurrencyTag> amount, Years holdingPeriod = 0) : value_(amount), holdingPeriod_(holdingPeriod) {}

    MonetaryAmount<T_CurrencyTag> getValue()  const noexcept override {return value_;}
    MonetaryAmount<T_CurrencyTag> getReturn(Years holdingPeriod) const
    {
        return getReturn();
    }
    MonetaryAmount<T_CurrencyTag> getReturn() const override
    {
        return {0};
    }
    std::string_view getName() const noexcept override {return name_;}

    std::int64_t getID() const noexcept override {return cashID_;}
    void addYearsToHoldingPeriod(Years years) override {holdingPeriod_ += years;}
    private: 
    MonetaryAmount<T_CurrencyTag> value_;
    Years holdingPeriod_;
    static inline std::int64_t cashIDNum {0};
    const std::int64_t cashID_{++cashIDNum};
    constexpr static std::string_view name_ {"Cash"}; // unlike other Financial Instruments, Cash is not a type of financial instrument that can be named by the user. It is always just "Cash"
};

class Portfolio
{
public: 
    Portfolio(std::string portfolioName, std::vector<std::unique_ptr<FinancialInstrument>> instruments) : portfolioName_(portfolioName), instruments_(std::move(instruments)) {}
    Portfolio (std::string portfolioName) : portfolioName_(portfolioName) {}
    Portfolio() : portfolioName_(defaultPortfolioName) {}

    const auto& getInstruments() const {return instruments_;} 
    std::string_view getPortfolioName() const noexcept {return portfolioName_;}
    std::int64_t getPortfolioID() const noexcept {return portfolioID_;}

    template <typename TargetCurrencyTag>
    MonetaryAmount<TargetCurrencyTag> getTotalValue(bool convertCurrency = true) const
    {
        auto valueFunction = [](const FinancialInstrument* instrument) { return instrument->getValue(); };
        return getTotalAccumulation<TargetCurrencyTag>(valueFunction, convertCurrency);
    }

    template <typename TargetCurrencyTag>
    MonetaryAmount<TargetCurrencyTag> getTotalReturn(bool convertCurrency = true) const
    {
        auto returnFunction = [](const FinancialInstrument* instrument) { return instrument->getReturn(); };
        return getTotalAccumulation<TargetCurrencyTag>(returnFunction, convertCurrency);
    }

    template <typename TargetCurrencyTag>
    MonetaryAmount<TargetCurrencyTag> getTotalReturn(Years holdingPeriod, bool convertCurrency = true) const
    {
        auto returnFunction = [holdingPeriod](const FinancialInstrument* instrument) { return instrument->getReturn(holdingPeriod); };
        return getTotalAccumulation<TargetCurrencyTag>(returnFunction, convertCurrency);
    }

    void addInstrument(std::unique_ptr<FinancialInstrument> instrument)
    {
        instruments_.push_back(std::move(instrument));
    }

    void removeInstrument(FinancialInstrument& instrument)
    {
        // TODO implement individual IDs for each FinancialInstrument rather than comparing pointers, as this is not a reliable way to identify unique instruments
        auto removeCondition = [&instrument](const std::unique_ptr<FinancialInstrument>& ptr) { return ptr.get() == &instrument; };
        
        auto newEnd = std::remove_if(instruments_.begin(), instruments_.end(), removeCondition);
        instruments_.erase(newEnd, instruments_.end());
        // C++20 syntax for the above would be:
        // std::erase_if(instruments, removeCondition);
    }


/*
// This is a convenience function, I shouldn't even include this in portfolio anyway (anti-pattern)
    std::vector<std::unique_ptr<FinancialInstrument>> getInstrumentsSortedByValue(bool ascending = true) const
    {
        std::vector<std::unique_ptr<FinancialInstrument>> sortedInstruments {std::copy(instruments_.begin(), instruments_.end(), std::back_inserter(sortedInstruments))};
        auto valueFunction = [](const std::unique_ptr<FinancialInstrument>& instrument) { return instrument->getValue(); };
        if (ascending)
        {
            std::sort(sortedInstruments.begin(), sortedInstruments.end(), [valueFunction](const auto& a, const auto& b) { return valueFunction(a) < valueFunction(b); });
        }
        else
        {
            std::sort(sortedInstruments.begin(), sortedInstruments.end(), [valueFunction](const auto& a, const auto& b) { return valueFunction(a) > valueFunction(b); });
        }
        return sortedInstruments;
    }
*/
private:
    std::string_view portfolioName_;
    std::vector<std::unique_ptr<FinancialInstrument>> instruments_;
    std::int64_t portfolioID_ {++portfolioIDNum};
    static inline std::int16_t portfolioIDNum {0};
    static constexpr std::string_view defaultPortfolioName {"Portfolio"};


    template <typename TargetCurrencyTag>
    MonetaryAmount<TargetCurrencyTag> getTotalAccumulation(std::function<RuntimeMonetaryAmount(const FinancialInstrument*)> accumulationFunction, bool convertCurrency = true) const
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
                if (convertCurrency) static_assert("Currency conversion not implemented yet");
                else throw std::runtime_error("Currency mismatch and conversion not allowed");
            }
        }
        return {totalPips};
    }
};

int main()
{
    return 0;
}