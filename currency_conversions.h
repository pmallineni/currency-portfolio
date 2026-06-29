#pragma once

#include <unordered_map>
#include <utility> 
#include <memory>
#include "currency_types.h"


struct CurrencyPairHash {
    std::size_t operator()(const std::pair<const Currency*, const Currency*>& p) const;
};
// TODO:: replace floating point arithmetic with fixed point.
class ICurrencyConverter
{
    public:
    virtual Rate getRate(const Currency& from, const Currency& to) const = 0;
    virtual ~ICurrencyConverter() = default;
};

using CurrencyMap = std::unordered_map<std::pair<const Currency*, const Currency*>, Rate, CurrencyPairHash>;

class PresetCurrencyConverter : public ICurrencyConverter
{
    public: 
    PresetCurrencyConverter(CurrencyMap exchangeRates) : rates_(exchangeRates) {};
    PresetCurrencyConverter() = default;
    void setRate(const Currency& from, const Currency& to, Rate rate);
    Rate getRate(const Currency& from, const Currency& to) const override;

    private: 
    CurrencyMap rates_;
    

};

class APICurrencyConverter : public ICurrencyConverter
{
    protected: 
    std::string apiKey_;

    explicit APICurrencyConverter(std::string_view key) : apiKey_(key) {}
};


class CurrencyConverterService
{
    public:
    static CurrencyConverterService& instance();

    void setConverter(std::unique_ptr<ICurrencyConverter> converter);

    Rate getRate(const Currency& from, const Currency& to) const;

    private:
    std::unique_ptr<ICurrencyConverter> converter_;
    CurrencyConverterService() = default;

};
