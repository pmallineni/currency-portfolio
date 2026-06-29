#include <stdexcept>
#include "currency_conversions.h"
std::size_t CurrencyPairHash::operator()(const std::pair<const Currency*, const Currency*>& p) const {
    return std::hash<const Currency*>{}(p.first) ^ 
            (std::hash<const Currency*>{}(p.second) << 1);
}

// TODO get rid of floating point arithmetic
void PresetCurrencyConverter::setRate(const Currency& from, const Currency& to, Rate rate)
{
    rates_.insert_or_assign({&from, &to}, rate);
    __int128_t amount = Rate::detail::getScale() * Rate::detail::getScale() / Rate::detail::getRaw(rate);
    Rate inverseRate = Rate::from_parts(amount);
    rates_.insert_or_assign({&to, &from}, inverseRate);
}

Rate PresetCurrencyConverter::getRate(const Currency& from, const Currency& to) const
{
    auto it = rates_.find({&from, &to});
    if (it == rates_.end())
        throw std::runtime_error("No conversion rate registered");
    return it->second;
}

CurrencyConverterService& CurrencyConverterService::instance()
{
    static CurrencyConverterService service;
    return service;
}

void CurrencyConverterService::setConverter(std::unique_ptr<ICurrencyConverter> converter)
{
    converter_ = std::move(converter);
}

Rate CurrencyConverterService::getRate(const Currency& from, const Currency& to) const
{
    if (!converter_)
        throw std::runtime_error("No currency converter set");
    return converter_->getRate(from, to);
}
