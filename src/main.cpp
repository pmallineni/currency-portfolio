#include <cassert>
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <stdexcept>


#include "portfolio.h"
#include "portfolio_views.h"

#include "random.h"


static void testMonetaryAmountOperations()
{
    USDAmount usdValue1 = 15000_USD; // 150 USD
    USDAmount usdValue2 = 5000_USD; // 50 USD
    std::cout << usdValue2 << '\n';

    assert((usdValue1 + usdValue2) == 20000_USD); // 200 dollars
    assert((usdValue1 - usdValue2) == 10000_USD); // 100 USD
    assert((usdValue1 * "0.1"_rate) == 1500_USD); // 15 USD
    assert(usdValue2 < usdValue1);

    RuntimeMonetaryAmount runtimeUsd{(862934_USD).getPipAmount(), US_DOLLAR};
    RuntimeMonetaryAmount runtimeJpy{(1000_JPY).getPipAmount(), JAPANESE_YEN}; // must put parentheses around 1000_JPY because parser

    auto convertedUsdToJpy = runtimeUsd.convertTo(JAPANESE_YEN);
    auto convertBack = convertedUsdToJpy.as(USD::instance);
    Pip conversion {(convertBack - runtimeUsd).getPipAmount()};
    assert( std::abs(conversion) <= 500 );
    std::cout << "conversion diff " << conversion << '\n';


    auto convertedJpyToUsd = static_cast<RuntimeMonetaryAmount>(10000_JPY).convertTo(USD::instance);
    auto convertBack2 = convertedJpyToUsd.in(JPY::instance);
    assert( std::abs((convertBack2 - 10000_JPY).getPipAmount()) <= 1 );
    std::cout << "conversion diff " << (convertBack2 - 10000_JPY).getPipAmount() << '\n';

    assert(convertedJpyToUsd.getCurrency().isoCode == "USD");
    assert((runtimeUsd != RuntimeMonetaryAmount{15001, US_DOLLAR}));

    bool caught = false;
    try
    {
        [[maybe_unused]] auto invalidSum = runtimeUsd + runtimeJpy;
    }
    catch (const std::runtime_error&)
    {
        caught = true;
    }
    assert(caught && "Runtime currency validation should throw on mismatched currencies");
}

static void testFinancialInstruments()
{
    auto stock = std::make_unique<Stock<USD>>(10000_USD, "AAPL", "Apple", 2.0);
    stock->setValue(12000_USD);
    assert(stock->getValue() == 12000_USD);
    assert(stock->getReturn() == 2000_USD);
    assert(stock->getReturn(1.0) == 2000_USD);

    bool caught = false;
    try
    {
        stock->getReturn(-1.0);
    }
    catch (const std::range_error&)
    {
        caught = true;
    }
    assert(caught && "Stock should throw for negative holding period");

    caught = false;
    try
    {
        stock->getReturn(3.0);
    }
    catch (const std::range_error&)
    {
        caught = true;
    }
    assert(caught && "Stock should throw for holding periods greater than available");

    auto bond = std::make_unique<Bond<USD>>(10000_USD, "0.05"_rate, 1.0, 2.0);
    assert(bond->getValue() == 10000_USD);
    assert(bond->getReturn() == 1000_USD);
    assert(bond->getReturn(1.5) == 500_USD);

    caught = false;
    try
    {
        bond->getReturn(-0.5);
    }
    catch (const std::range_error&)
    {
        caught = true;
    }
    assert(caught && "Bond should throw for negative holding period");

    auto cash = std::make_unique<Cash<USD>>(5000_USD, 1.0);
    assert(cash->getValue() == 5000_USD);
    assert(cash->getReturn() == 0_USD);
    assert(cash->getReturn(10.0) == 0_USD);
}

static void testPortfolioAndViews()
{
    Portfolio portfolio{"Test Portfolio"};

    auto stock = std::make_unique<Stock<USD>>(10000_USD, "AAPL", "Apple", 2.0);
    stock->setValue(12000_USD);
    auto stockPtr = stock.get();

    auto bond = std::make_unique<Bond<USD>>(10000_USD, "0.05"_rate, 1.0, 2.0);
    auto bondPtr = bond.get();

    auto cash = std::make_unique<Cash<USD>>(5000_USD, 0.0);
    auto cashPtr = cash.get();

    assert(portfolio.addInstrument(std::move(stock)));
    assert(portfolio.addInstrument(std::move(bond)));
    assert(portfolio.addInstrument(std::move(cash)));
    assert(!portfolio.addInstrument(nullptr));
    assert(portfolio.getInstruments().size() == 3);

//    assert(portfolio.getTotalValue<USD>().getPipAmount() == 2700000);
//    assert(portfolio.getTotalReturn<USD>().getPipAmount() == 300000);
//    assert(portfolio.getTotalReturn<USD>(1.0).getPipAmount() == 250000);

    auto byValueAsc = PortfolioViews::getByValue(portfolio, true);
    assert(byValueAsc.size() == 3);
    assert(byValueAsc[0]->getName() == "Cash");
    assert(byValueAsc[1]->getName() == "Bond");
    assert(byValueAsc[2]->getName() == "Apple");

    auto byReturnDesc = PortfolioViews::getByReturn(portfolio, false);
    assert(byReturnDesc[0]->getName() == "Apple");
    assert(byReturnDesc[1]->getName() == "Bond");
    assert(byReturnDesc[2]->getName() == "Cash");

    auto byReturnHoldingPeriod = PortfolioViews::getByReturn(portfolio, 1.0, true);
    assert(byReturnHoldingPeriod[0]->getName() == "Cash");
    assert(byReturnHoldingPeriod[1]->getName() == "Bond");
    assert(byReturnHoldingPeriod[2]->getName() == "Apple");

    auto byNameAsc = PortfolioViews::getByName(portfolio, true);
    assert(byNameAsc[0]->getName() == "Apple");
    assert(byNameAsc[1]->getName() == "Bond");
    assert(byNameAsc[2]->getName() == "Cash");

    auto byIDAsc = PortfolioViews::getByID(portfolio, true);
    assert(std::is_sorted(byIDAsc.begin(), byIDAsc.end(), [](const FinancialInstrument* a, const FinancialInstrument* b){ return a->getID() < b->getID(); }));


    auto japanStock = std::make_unique<Stock<JPY>>(992309409_JPY, "JPNO", "JapanStock", 2);
    auto japanStockptr = japanStock.get();
    portfolio.addInstrument(std::move(japanStock));
    std::cout << portfolio.getTotalValue<USD>() << '\n';
    std::cout << portfolio.getTotalValue<JPY>() << '\n';
    std::cout << portfolio.getTotalValue<GBP>() << '\n';
    
    auto byReturnHoldingPeriod2 = PortfolioViews::getByValue(portfolio, 2.0);
    for (auto* instrument : byReturnHoldingPeriod2)
        if (instrument) std::cout << instrument->getName() << '\n';

    portfolio.removeInstrument(*stockPtr);
    assert(portfolio.getInstruments().size() == 3);

    portfolio.removeInstrument(*bondPtr);
    portfolio.removeInstrument(*cashPtr);
    portfolio.removeInstrument(*japanStockptr);
    assert(portfolio.getInstruments().empty());
}

int main()
{
    CurrencyInit::setupConversions();
    std::cout << "Running simple test suite...\n";
    testMonetaryAmountOperations();
    testFinancialInstruments();
    testPortfolioAndViews();
    std::cout << "All tests passed.\n";
    return 0;
}
