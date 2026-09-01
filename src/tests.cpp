// Test suite for the fixed-point currency / portfolio library.
// Build/run separately from main.cpp (see CMakeLists.txt -> `Tests` target).
//
// These tests aim to check what the code is *supposed* to do (documented
// behavior, boundary conditions, invariants) rather than just re-confirming
// whatever the current implementation happens to output.

#include <iostream>
#include <memory>
#include <limits>

#include "test_framework.h"
#include "portfolio.h"
#include "portfolio_views.h"
#include "currency_conversions.h"

using namespace TestFramework;

static void testRateParsing()
{
    runSection("-- Rate::from_string --", []()
    {
        // Whole numbers with no decimal point should parse as if ".0"
        Rate whole = Rate::from_string("5");
        Rate wholeExplicit = Rate::from_string("5.0");
        CHECK_MSG(Rate::detail::getRaw(whole) == Rate::detail::getRaw(wholeExplicit),
                  "\"5\" should parse the same as \"5.0\"");

        // Fractional digits beyond the internal scale (8 decimal places)
        // should be truncated, not overflow or throw.
        CHECK_NO_THROW(Rate::from_string("0.123456789123"));

        // Fewer digits than the scale should be right-padded with zero, not left unscaled.
        Rate shortFrac = Rate::from_string("0.1");
        Rate longFrac  = Rate::from_string("0.10000000");
        CHECK_MSG(Rate::detail::getRaw(shortFrac) == Rate::detail::getRaw(longFrac),
                  "\"0.1\" should equal \"0.10000000\" once padded to full scale");
    });
}

static void testMonetaryAmountOperations()
{
    runSection("-- MonetaryAmount --", []()
    {
        USDAmount usdValue1 = 15000_USD; // 150.00
        USDAmount usdValue2 = 5000_USD;  // 50.00

        CHECK((usdValue1 + usdValue2) == 20000_USD);
        CHECK((usdValue1 - usdValue2) == 10000_USD);
        CHECK((usdValue1 * "0.1"_rate) == 1500_USD);
        CHECK(usdValue2 < usdValue1);

        // operator*= was previously broken (assigned to a member function
        // pointer and read a private static member). Confirm it now behaves
        // identically to non-mutating operator*.
        USDAmount mutated = usdValue1;
        mutated *= "0.1"_rate;
        CHECK_MSG(mutated == 1500_USD, "operator*=(Rate) should scale in place like operator*");

        // Currency literal range check: a value large enough to overflow
        // Pip once scaled by unitPips should throw, not silently wrap.
        CHECK_THROWS((void)(20000000000000000_USD));
    });
}

static void testRuntimeMonetaryAmount()
{
    runSection("-- RuntimeMonetaryAmount --", []()
    {
        RuntimeMonetaryAmount usd{15000, US_DOLLAR};
        RuntimeMonetaryAmount jpy{100000, JAPANESE_YEN};

        // Mismatched-currency arithmetic and comparisons must fail loudly,
        // not silently compare/combine raw pip counts.
        CHECK_THROWS((void)(usd + jpy));
        CHECK_THROWS((void)(usd - jpy));
        CHECK_THROWS((void)(usd == jpy));
        CHECK_THROWS((void)(usd < jpy));

        RuntimeMonetaryAmount usd2{15000, US_DOLLAR};
        CHECK_NO_THROW((void)(usd == usd2));
        CHECK(usd == usd2);
    });
}

static void testCurrencyConversion()
{
    runSection("-- Currency conversion --", []()
    {
        // A converter with no rate registered in either direction should
        // report that clearly via hasRate, and getRate should still throw
        // (rather than convertTo silently guessing or looping forever).
        PresetCurrencyConverter emptyConverter;
        CHECK(!emptyConverter.hasRate(US_DOLLAR, EURO));
        CHECK(!emptyConverter.hasRate(EURO, US_DOLLAR));
        CHECK_THROWS(emptyConverter.getRate(US_DOLLAR, EURO));

        // Only the forward direction is registered; hasRate should reflect
        // that asymmetry rather than reporting both directions as available.
        PresetCurrencyConverter oneWay;
        oneWay.setRate(EURO, US_DOLLAR, Rate::from_string("1.1"));
        CHECK(oneWay.hasRate(EURO, US_DOLLAR));
        CHECK(!oneWay.hasRate(US_DOLLAR, EURO));

        // convertTo must resolve correctly using only the reverse-registered
        // rate (GBP->JPY exists, JPY->GBP does not, in CurrencyInit setup).
        RuntimeMonetaryAmount tenThousandJpy{1000000, JAPANESE_YEN}; // 10,000 yen (unit pip = 10000)
        RuntimeMonetaryAmount inGbp = tenThousandJpy.convertTo(BRITISH_POUND);
        CHECK(inGbp.getCurrency().isoCode == "GBP");

        RuntimeMonetaryAmount roundTrip = inGbp.convertTo(JAPANESE_YEN);
        Pip diff = std::abs((roundTrip - tenThousandJpy).getPipAmount());
        CHECK_MSG(diff <= 2, "round-trip JPY -> GBP -> JPY should be within rounding tolerance");

        // Converting to the currency it's already in should be a no-op and
        // must not require any rate to be registered.
        CHECK_NO_THROW((void)tenThousandJpy.convertTo(JAPANESE_YEN));
    });
}

static void testFinancialInstruments()
{
    runSection("-- Financial instruments --", []()
    {
        auto stock = std::make_unique<Stock<USD>>(10000_USD, "AAPL", "Apple", 2.0);
        stock->setValue(12000_USD);
        CHECK(stock->getValue() == 12000_USD);
        CHECK(stock->getReturn() == 2000_USD);

        // Boundary: holding period exactly equal to the stock's holding
        // period should be valid (only *exceeding* it should throw).
        CHECK_NO_THROW((void)stock->getReturn(2.0));
        CHECK_THROWS((void)stock->getReturn(2.0001));
        CHECK_THROWS((void)stock->getReturn(-0.01));

        // Bond: simple interest should only accrue on whole elapsed payout
        // periods (floor division), not proportionally within a period.
        auto bond = std::make_unique<Bond<USD>>(10000_USD, Rate::from_string("0.05"), 1.0, 2.0);
        CHECK(bond->getReturn(0.99) == 0_USD);      // just under one payout period: nothing accrued yet
        CHECK(bond->getReturn(1.0) == 500_USD);     // exactly one period: 5% of 100.00
        CHECK(bond->getReturn(1.99) == 500_USD);    // still in the same period
        CHECK(bond->getReturn(2.0) == 1000_USD);    // two full periods
        CHECK_THROWS((void)bond->getReturn(-0.5));

        auto cash = std::make_unique<Cash<USD>>(5000_USD, 1.0);
        CHECK(cash->getReturn() == 0_USD);
        CHECK(cash->getReturn(100.0) == 0_USD); // cash never accrues, regardless of period
    });
}

static void testPortfolio()
{
    runSection("-- Portfolio --", []()
    {
        Portfolio portfolio{"Test Portfolio"};

        CHECK_MSG(!portfolio.addInstrument(nullptr), "adding a null instrument should be rejected, not stored");

        auto stock = std::make_unique<Stock<USD>>(10000_USD, "AAPL", "Apple", 1.0);
        stock->setValue(11000_USD);
        portfolio.addInstrument(std::move(stock));

        auto yen = std::make_unique<Stock<JPY>>(1000000_JPY, "JPNO", "JapanStock", 1.0);
        portfolio.addInstrument(std::move(yen));

        CHECK(portfolio.getInstruments().size() == 2);

        // With conversion allowed (the default), mixed-currency instruments
        // should combine into a single total without throwing.
        CHECK_NO_THROW((void)portfolio.getTotalValue<USD>());

        // With conversion explicitly disabled, a portfolio holding more than
        // one currency must refuse to produce a total rather than silently
        // summing incompatible pip counts.
        CHECK_THROWS((void)(portfolio.getTotalValue<USD, false>()));
    });
}

static void testPortfolioViews()
{
    runSection("-- Portfolio views --", []()
    {
        Portfolio portfolio{"Views Portfolio"};

        auto stock = std::make_unique<Stock<USD>>(10000_USD, "AAPL", "Apple", 2.0);
        stock->setValue(12000_USD);
        auto bond = std::make_unique<Bond<USD>>(10000_USD, Rate::from_string("0.05"), 1.0, 2.0);
        auto cash = std::make_unique<Cash<USD>>(5000_USD, 0.0);

        portfolio.addInstrument(std::move(stock));
        portfolio.addInstrument(std::move(bond));
        portfolio.addInstrument(std::move(cash));

        auto byValueAsc = PortfolioViews::getByValue(portfolio, true);
        CHECK(byValueAsc.size() == 3);
        CHECK(byValueAsc.front()->getName() == "Cash");
        CHECK(byValueAsc.back()->getName() == "Apple");

        auto byNameAsc = PortfolioViews::getByName(portfolio, true);
        CHECK(byNameAsc[0]->getName() == "Apple");
        CHECK(byNameAsc[1]->getName() == "Bond");
        CHECK(byNameAsc[2]->getName() == "Cash");

        auto byIdAsc = PortfolioViews::getByID(portfolio, true);
        CHECK(std::is_sorted(byIdAsc.begin(), byIdAsc.end(),
              [](const FinancialInstrument* a, const FinancialInstrument* b) { return a->getID() < b->getID(); }));
    });
}

int main()
{
    CurrencyInit::setupConversions();

    testRateParsing();
    testMonetaryAmountOperations();
    testRuntimeMonetaryAmount();
    testCurrencyConversion();
    testFinancialInstruments();
    testPortfolio();
    testPortfolioViews();

    return summarize();
}
