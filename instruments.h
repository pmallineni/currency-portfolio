#pragma once

#include <atomic>

#include "currencies.h"


class FinancialInstrument
{
    protected:
    static std::uint64_t generateID() noexcept
    {
        static std::atomic<std::uint64_t> counter {0};
        return ++counter;
    }

    const std::uint64_t id_ {generateID()};

    public:
    std::uint64_t getID() const noexcept { return id_; }

    virtual RuntimeMonetaryAmount getValue() const  = 0;
    virtual RuntimeMonetaryAmount getReturn() const = 0;
    virtual RuntimeMonetaryAmount getReturn(Years holdingPeriod) const = 0;
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



    MonetaryAmount<T_CurrencyTag> getSpecificValue() const override {return value_;}
    std::string_view getName() const noexcept override {return name_;}
    std::string_view getTickerSymbol() const noexcept {return tickerSymbol_;}

    MonetaryAmount<T_CurrencyTag> getSpecificReturn() const override
    {
        return MonetaryAmount<T_CurrencyTag>(value_.getPipAmount() - priceBought_.getPipAmount());
    }
    
    // TODO: implement a more accurate return calculation that implements some sort of arbitrary distribution (normal distribution by default) taking account inputted holding period
    MonetaryAmount<T_CurrencyTag> getSpecificReturn(Years holdingPeriod) const override
    {
        if (holdingPeriod < 0) throw (std::range_error("holdingPeriod for a financial return cannot be less than 0"));
        if (holdingPeriod > holdingPeriod_) throw (std::range_error("holdingPeriod for a financial return cannot be greater than the holding period of the stock"));

        return MonetaryAmount<T_CurrencyTag>(value_.getPipAmount() - priceBought_.getPipAmount());
    }
    
    void setValue(MonetaryAmount<T_CurrencyTag> newValue) {value_ = newValue;}
    void addYearsToHoldingPeriod(Years years) {holdingPeriod_ += years;}
    private: 
    MonetaryAmount<T_CurrencyTag> priceBought_;
    MonetaryAmount<T_CurrencyTag> value_;
    std::string_view tickerSymbol_;
    std::string_view name_;
    Years holdingPeriod_;
    
    
};

template <typename T_CurrencyTag>
class Bond : public TypedFinancialInstrument<T_CurrencyTag>
{
    public:
    Bond(MonetaryAmount<T_CurrencyTag> principal, double couponRate, Years payoutPeriod = 0.5, Years holdingPeriod = 0) :
    principal_(principal), value_(principal), couponRate_(couponRate), payoutPeriod_(payoutPeriod), holdingPeriod_(holdingPeriod), name_(defaultName){}
    
    Bond(MonetaryAmount<T_CurrencyTag> principal, double couponRate, std::string name, Years payoutPeriod = 0.5, Years holdingPeriod = 0) :
    principal_(principal), value_(principal), couponRate_(couponRate), payoutPeriod_(payoutPeriod), holdingPeriod_(holdingPeriod), name_(name) {}

    MonetaryAmount<T_CurrencyTag> getSpecificValue() const override {return value_;} // maybe modifiable if I include buy and sell
    MonetaryAmount<T_CurrencyTag> getSpecificReturn(Years holdingPeriod) const
    {
        if (holdingPeriod < 0) throw (std::range_error("holdingPeriod for a financial return cannot be less than 0"));

        std::int64_t numInterestPeriods = static_cast<std::int64_t>(std::floor(holdingPeriod / payoutPeriod_));
        return MonetaryAmount<T_CurrencyTag>(principal_ * numInterestPeriods * couponRate_); // simple Interest
    }
    MonetaryAmount<T_CurrencyTag> getSpecificReturn() const override
    {
        return getSpecificReturn(holdingPeriod_);
    }

    std::string_view getName() const noexcept override {return name_;}

    void addYearsToHoldingPeriod(Years years) {holdingPeriod_ += years;}


    private:
    MonetaryAmount<T_CurrencyTag> principal_; 
    MonetaryAmount<T_CurrencyTag> value_;
    double couponRate_;
    Years payoutPeriod_;
    Years holdingPeriod_;
    std::string_view name_;

    static constexpr std::string_view defaultName {"Bond"};


};

template <typename T_CurrencyTag>
class Cash : public TypedFinancialInstrument<T_CurrencyTag>
{
    public: 
    Cash(MonetaryAmount<T_CurrencyTag> amount, Years holdingPeriod = 0) : value_(amount), holdingPeriod_(holdingPeriod) {}

    MonetaryAmount<T_CurrencyTag> getSpecificValue()  const noexcept override {return value_;}
    MonetaryAmount<T_CurrencyTag> getSpecificReturn(Years holdingPeriod) const
    {
        return getSpecificReturn();
    }
    MonetaryAmount<T_CurrencyTag> getSpecificReturn() const override
    {
        return MonetaryAmount<T_CurrencyTag>{0};
    }
    std::string_view getName() const noexcept override {return name_;}

    void addYearsToHoldingPeriod(Years years) override {holdingPeriod_ += years;}
    private: 
    MonetaryAmount<T_CurrencyTag> value_;
    Years holdingPeriod_;
    constexpr static std::string_view name_ {"Cash"}; // unlike other Financial Instruments, Cash is not a type of financial instrument that can be named by the user. It is always just "Cash"
};

struct InstrumentFunctions
{
    static constexpr auto idFunction =     [](const FinancialInstrument* instrument) {return instrument->getID(); };
    static constexpr auto nameFunction =   [](const FinancialInstrument* instrument) {return instrument->getName(); };
    static constexpr auto valueFunction =  [](const FinancialInstrument* instrument) { return instrument->getValue(); };
    static constexpr auto returnFunction = [](const FinancialInstrument* instrument) { return instrument->getReturn(); };

    static constexpr auto holdingPeriodReturnFunction = [](Years holdingPeriod)
    {
        return [holdingPeriod](const FinancialInstrument* instrument) { return instrument->getReturn(holdingPeriod);};
    };
};