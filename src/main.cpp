// Small usage demo for the fixed-point currency / portfolio library.
// See src/tests.cpp (built as the `Tests` target) for the actual test suite.

#include <iostream>

#include "portfolio.h"
#include "portfolio_views.h"

int main()
{
    CurrencyInit::setupConversions();

    Portfolio portfolio{"Demo Portfolio"};

    auto stock = std::make_unique<Stock<USD>>(10000_USD, "AAPL", "Apple", 2.0);
    stock->setValue(15000_USD);
    portfolio.addInstrument(std::move(stock));

    auto bond = std::make_unique<Bond<USD>>(20000_USD, "0.05"_rate, "Treasury Note", 1.0, 3.0);
    portfolio.addInstrument(std::move(bond));

    auto yenCash = std::make_unique<Cash<JPY>>(500000_JPY);
    portfolio.addInstrument(std::move(yenCash));

    std::cout << portfolio.getPortfolioName() << " total value (USD): "
              << portfolio.getTotalValue<USD>() << '\n';
    std::cout << portfolio.getPortfolioName() << " total value (JPY): "
              << portfolio.getTotalValue<JPY>() << '\n';

    std::cout << "\nHoldings by return (highest first):\n";
    for (const auto* instrument : PortfolioViews::getByReturn(portfolio, false))
    {
        RuntimeMonetaryAmount ret = instrument->getReturn();
        std::cout << "  " << instrument->getName() << ": "
                  << ret.getCurrency().symbol << ret.getPipAmount() << " (raw pips)\n";
    }

    return 0;
}
