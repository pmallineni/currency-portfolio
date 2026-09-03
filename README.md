# currency-portfolio

A small C++17 library for representing money as **fixed-point integers**, converting between currencies, and modeling a portfolio of financial instruments (stocks, bonds, cash) with typed and runtime-checked arithmetic.

## Table of contents

- [Why fixed-point?](#why-fixed-point)
- [Requirements](#requirements)
- [Building](#building)
- [Project layout](#project-layout)
- [Core concepts](#core-concepts)
  - [Currency](#currency)
  - [Rate](#rate)
  - [Pip](#pip)
  - [MonetaryAmount\<Tag\> vs RuntimeMonetaryAmount](#monetaryamounttag-vs-runtimemonetaryamount)
- [Currency conversion](#currency-conversion)
- [Financial instruments](#financial-instruments)
- [Portfolio](#portfolio)
- [Portfolio views](#portfolio-views)
- [Usage example](#usage-example)
- [Testing](#testing)
- [Known limitations / TODOs in the code](#known-limitations--todos-in-the-code)

---

## Why fixed-point?

Money should never be stored as `float`/`double` — rounding error compounds silently. This library instead stores every amount as an integer count of **pips** (the smallest representable unit of a currency) and does all rate math with a fixed-point `Rate` type, promoting to `__int128_t` internally so that multiplying two 64-bit fixed-point numbers together doesn't overflow before it's scaled back down.

Both `MonetaryAmount<Tag>::operator*(double)` and `operator*(float)` are explicitly `= delete`d, so the type system stops you from ever multiplying money by a floating point number — you're forced to go through `Rate` instead.

## Requirements

- **C++17** compiler
- **GCC** (or another compiler with `__int128_t`/`__int128` support). The 128-bit intermediate type used for overflow-safe rate math is a GCC/Clang extension, not standard C++, which is why a GCC-family compiler is required (MSVC does not support it).
- CMake ≥ 3.23

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

This produces two executables (see `CMakeLists.txt`):

| Target       | Sources                                                                                   | Purpose                          |
|--------------|--------------------------------------------------------------------------------------------|-----------------------------------|
| `Executable` | `main.cpp`, `portfolio.cpp`, `portfolio_views.cpp`, `currency_conversions.cpp`             | Small usage demo                 |
| `Tests`      | `tests.cpp`, `portfolio.cpp`, `portfolio_views.cpp`, `currency_conversions.cpp`             | Self-contained unit test suite   |

Run them directly, or via CTest:

```bash
./Executable
./Tests
ctest        # registers Tests as "UnitTests"
```

If you don't have CMake available, you can compile directly with g++:

```bash
g++ -std=c++17 -Iinclude src/main.cpp src/portfolio.cpp src/portfolio_views.cpp src/currency_conversions.cpp -o Executable
g++ -std=c++17 -Iinclude src/tests.cpp src/portfolio.cpp src/portfolio_views.cpp src/currency_conversions.cpp -o Tests
```

The `CMAKE_BUILD_TYPE` is hard-set to `Debug`, and `-gdwarf-4` is added for Debug builds specifically to keep debug info compatible with CodeLLDB (the comment in `CMakeLists.txt` notes it can be swapped for `-g1` if the debugger keeps crashing on variable parsing).

## Project layout

```
include/
  currency_types.h        Currency struct, CurrencyTag, Rate (fixed-point rate type)
  currency_conversions.h  ICurrencyConverter interface, PresetCurrencyConverter, CurrencyConverterService
  currencies.h            Concrete currencies (USD, EUR, GBP, JPY), literals, preset rate setup
  money_amounts.h         MonetaryAmount<Tag> (compile-time currency) and RuntimeMonetaryAmount
  instruments.h           FinancialInstrument hierarchy: Stock, Bond, Cash
  portfolio.h             Portfolio class (owns instruments, aggregates totals)
  portfolio_views.h       Read-only sorted "views" over a Portfolio
  random.h                Standalone Mersenne Twister helper (not currently used elsewhere)
  test_framework.h        Minimal CHECK/CHECK_THROWS test macros

src/
  currency_conversions.cpp   Implementation of the conversion classes
  portfolio.cpp               Implementation of Portfolio
  portfolio_views.cpp          Implementation of the PortfolioViews:: free functions
  main.cpp                     Demo program
  tests.cpp                    Unit tests (built as the `Tests` target)
```

## Core concepts

### Currency

`Currency` (`currency_types.h`) is a plain, non-copyable struct describing a currency:

```cpp
struct Currency {
    const std::string_view name;
    const char* symbol;
    const std::string_view isoCode;
    const int unitPips;   // how many "pips" make up one major unit of this currency
};
```

`currencies.h` defines the four currencies the library ships with:

| Currency | Tag   | `unitPips` |
|----------|-------|------------|
| US Dollar | `USD` | 1,000,000 |
| Euro      | `EUR` | 1,000,000 |
| British Pound | `GBP` | 1,000,000 |
| Japanese Yen  | `JPY` | 10,000 |

`unitPips` lets each currency define its own precision — Yen (which has no minor subunit in everyday use) uses a smaller scale than USD/EUR/GBP.

Each currency also gets a `CurrencyTag<Instance>` (e.g. `using USD = CurrencyTag<US_DOLLAR>;`), a compile-time handle used as a template parameter so that the currency of a `MonetaryAmount` is checked **at compile time**.

### Rate

`Rate` (`currency_types.h`) is a fixed-point decimal used for exchange rates and interest rates:

- Internally stored as an `int64_t` scaled by `SCALE = 10^8` (8 decimal digits of precision).
- Constructed via the `_rate` user-defined literal or `Rate::from_string(...)`, e.g. `"0.05"_rate`.
- `Rate::from_string` is `constexpr` and parses the string digit-by-digit (no `std::stod`/floating point involved), padding or truncating the fractional part to exactly 8 digits.
- The private nested `Rate::detail` struct exposes `getRaw()`/`getScale()` so other headers in the library can do fixed-point math without making the whole raw representation public.

### Pip

`Pip` is just `using Pip = std::int64_t;` — the smallest indivisible unit of a `MonetaryAmount`. A "pip" here doesn't mean 1/100 of a unit like in FX trading conventions; its size is defined per-currency by `Currency::unitPips` (e.g. for USD, 1,000,000 pips = 1 dollar).

### `MonetaryAmount<Tag>` vs `RuntimeMonetaryAmount`

The library provides **two** representations of money, defined in `money_amounts.h`:

- **`MonetaryAmount<T_CurrencyTag>`** — a compile-time-typed amount (e.g. `MonetaryAmount<USD>`). The currency is a template parameter, so mixing `MonetaryAmount<USD>` and `MonetaryAmount<EUR>` in an operator won't even compile. This is the type you want whenever the currency of a value is known statically (e.g. "the price a stock was bought at").
- **`RuntimeMonetaryAmount`** — holds a `Pip` amount plus a `const Currency*`, checked **at runtime**. Every `MonetaryAmount<Tag>` implicitly converts to a `RuntimeMonetaryAmount`, which is how the library lets heterogeneous, dynamically-typed collections of money (e.g. instruments in a mixed-currency `Portfolio`) interoperate. Mismatched-currency arithmetic on `RuntimeMonetaryAmount` throws `std::runtime_error` via `validateCurrency(...)`.

Both types support `+`, `-`, comparisons, and multiplication by a `Rate` or an integer scalar — multiplication by `float`/`double` is explicitly deleted on both types to force fixed-point-safe code.

`MonetaryAmount<Tag>::percentOf(Rate)` (used by `operator*`) and `RuntimeMonetaryAmount`'s equivalent `getPercentOf(...)` both widen the pip amount to `__int128_t` before multiplying by the raw rate, and only narrow back down to `Pip` (`int64_t`) after dividing by `Rate::detail::getScale()`. This is the reason the project requires a compiler that supports the 128-bit integer extension.

`RuntimeMonetaryAmount::convertTo(...)` (aliased as `.in(...)` / `.as(...)`) performs currency conversion, described next.

## Currency conversion

`currency_conversions.h` / `.cpp` define the conversion machinery:

- **`ICurrencyConverter`** — abstract interface with `getRate(from, to)` and `hasRate(from, to)`.
- **`PresetCurrencyConverter`** — a concrete implementation backed by an `unordered_map` keyed on `{const Currency*, const Currency*}` pairs (hashed via `CurrencyPairHash`). Rates are only stored in one direction; the reverse direction is derived mathematically rather than stored redundantly (see below).
- **`APICurrencyConverter`** — an abstract base with just a stored API key; there's no concrete implementation yet, so this is a placeholder for a future live-rate provider.
- **`CurrencyConverterService`** — a process-wide singleton (`CurrencyConverterService::instance()`) that holds a `std::unique_ptr<ICurrencyConverter>`, so the whole application shares one configured converter (preset, API-backed, or otherwise) instead of passing one around explicitly.

`currencies.h`'s `CurrencyInit::setupConversions()` builds a `PresetCurrencyConverter` with hardcoded EUR/USD/JPY/GBP cross rates and installs it into `CurrencyConverterService`. Call this once at program startup (both `main.cpp` and `tests.cpp` do this first thing) before doing any cross-currency conversion.

`RuntimeMonetaryAmount::convertTo(target)`:
1. If already in the target currency, returns `*this` unchanged.
2. If the service has a direct rate `from → to`, multiplies by the rate.
3. Otherwise, if the service has the *reverse* rate `to → from`, it divides by that rate instead — so you only need to register one direction of each currency pair.
4. If neither exists, throws `std::runtime_error("No conversion rate registered between currencies")`.

All of the intermediate arithmetic in this conversion is done in `__int128_t` to avoid overflow when multiplying a large pip amount by a scaled rate before dividing back down.

## Financial instruments

`instruments.h` defines a small instrument hierarchy:

- **`FinancialInstrument`** — abstract base. Every instrument gets an auto-incrementing, thread-safe unique ID (`std::atomic<uint64_t>` counter) and exposes `getValue()`, `getReturn()`, `getReturn(Years)`, `getName()`, and `addYearsToHoldingPeriod(Years)`, all in terms of `RuntimeMonetaryAmount` so instruments in different currencies can live in the same collection.
- **`TypedFinancialInstrument<Tag>`** — a CRTP-free typed layer that implements the `FinancialInstrument` virtuals by delegating to currency-typed `getSpecificValue()`/`getSpecificReturn()` methods that subclasses must implement, then wraps the result as a `RuntimeMonetaryAmount`.
- **`Stock<Tag>`** — tracks `priceBought_` and current `value_`. Return is simply `value_ - priceBought_` (the `holdingPeriod` overload currently computes the same thing after validating the requested period is within range — see [Known limitations](#known-limitations--todos-in-the-code)).
- **`Bond<Tag>`** — tracks `principal_`, a `couponRate_` (a `Rate`), and a `payoutPeriod_` in years. `getSpecificReturn(holdingPeriod)` computes simple interest: `principal * floor(holdingPeriod / payoutPeriod) * couponRate`.
- **`Cash<Tag>`** — a flat balance with no interest; `getSpecificReturn()` is always zero. Its name is always `"Cash"` — the user cannot rename it.

`Years` is `using Years = double;` — the one place in the library that intentionally *isn't* fixed-point, since holding periods are approximate durations rather than exact currency values.

`InstrumentFunctions` is a small struct of reusable stateless lambdas (`idFunction`, `nameFunction`, `valueFunction`, `returnFunction`, and a `holdingPeriodReturnFunction` factory) used to avoid repeating "call this method on a `FinancialInstrument*`" all over `Portfolio` and `PortfolioViews`.

## Portfolio

`Portfolio` (`portfolio.h` / `.cpp`) owns a `std::vector<std::unique_ptr<FinancialInstrument>>` and:

- Is move-only (copy constructor/assignment deleted) since it owns unique instrument pointers.
- Assigns every portfolio a unique, monotonically increasing `portfolioID_`.
- `addInstrument(unique_ptr<FinancialInstrument>)` — takes ownership; returns `false` if passed `nullptr` instead of adding it.
- `removeInstrument(FinancialInstrument&)` — removes by matching `getID()` (erase-remove idiom; the code notes a C++20 `std::erase_if` equivalent).
- `getTotalValue<TargetCurrencyTag, ConvertCurrency=true>()` / `getTotalReturn<...>()` (with or without a `Years` holding period) — sums every instrument's value/return, converting into `TargetCurrencyTag` as needed. If `ConvertCurrency` is set to `false` and an instrument isn't already in the target currency, it throws `std::runtime_error` instead of silently converting — useful when you want to assert a portfolio is single-currency.
- `checkInvariants()` — a private debug helper (`assert`) that verifies no `nullptr` instrument ever ends up stored.

## Portfolio views

`portfolio_views.h` / `.cpp` provide read-only, non-owning **sorted views** over a `Portfolio` without duplicating or mutating the underlying data — each view is a fresh `std::vector<const FinancialInstrument*>` of pointers into the portfolio's own instruments.

Public entry points (`PortfolioViews::`):

- `getByValue(portfolio, ascending = true)`
- `getByReturn(portfolio, ascending = true)`
- `getByReturn(portfolio, holdingPeriod, ascending = true)`
- `getByID(portfolio, ascending = true)`
- `getByName(portfolio, lettersAtoZ = true)`

Internally these are built from two generic helpers in `PortfolioViews::details`:

- `getSortedInstrumentByFunc(...)` — sorts by any scalar-returning function (ID, name).
- `getSortedInstrumentbyMoneyFunc<..., ConvertCurrency=true>(...)` — sorts by a function that returns `RuntimeMonetaryAmount`; a `static_assert` enforces the function's return type at compile time (with a helpful message pointing you to the other helper if you pass the wrong kind of function). When comparing two instruments in different currencies, it converts the right-hand side into the left-hand side's currency before comparing, so sort order is always financially meaningful rather than comparing raw pip counts across currencies.

## Usage example

From `src/main.cpp`:

```cpp
#include "portfolio.h"
#include "portfolio_views.h"

int main()
{
    CurrencyInit::setupConversions();               // register preset FX rates

    Portfolio portfolio{"Demo Portfolio"};

    auto stock = std::make_unique<Stock<USD>>(10000_USD, "AAPL", "Apple", 2.0);
    stock->setValue(15000_USD);
    portfolio.addInstrument(std::move(stock));

    auto bond = std::make_unique<Bond<USD>>(20000_USD, "0.05"_rate, "Treasury Note", 1.0, 3.0);
    portfolio.addInstrument(std::move(bond));

    auto yenCash = std::make_unique<Cash<JPY>>(500000_JPY);
    portfolio.addInstrument(std::move(yenCash));

    std::cout << portfolio.getTotalValue<USD>() << '\n';   // converts JPY cash into USD
    std::cout << portfolio.getTotalValue<JPY>() << '\n';   // converts everything into JPY

    for (const auto* instrument : PortfolioViews::getByReturn(portfolio, false))
        std::cout << instrument->getName() << ": " << instrument->getReturn().getPipAmount() << '\n';
}
```

Currency literals (`_USD`, `_EUR`, `_GBP`, `_JPY`) are provided in `currencies.h` for convenient construction, e.g. `10000_USD`. They throw `std::range_error` if the literal value would overflow `Pip` once scaled by `unitPips`.

## Testing

`src/tests.cpp` is a self-contained suite built with the header-only harness in `include/test_framework.h` (`CHECK`, `CHECK_MSG`, `CHECK_THROWS`, `CHECK_NO_THROW` macros — no external test framework dependency). It covers, in order:

1. `Rate::from_string` parsing (whole numbers, decimals, padding/truncating fractional digits)
2. `MonetaryAmount<Tag>` arithmetic and comparisons
3. `RuntimeMonetaryAmount` behavior, including currency-mismatch exceptions
4. Currency conversion (direct rate, reverse-derived rate, missing-rate error)
5. `Stock`, `Bond`, and `Cash` value/return calculations
6. `Portfolio` (adding/removing instruments, aggregated totals, currency mismatch without conversion)
7. `PortfolioViews` sorting (by value, return, ID, name, ascending/descending)

Run it with `./Tests` after building, or via `ctest` (registered as test `UnitTests`).

## Known limitations / TODOs in the code

- **`APICurrencyConverter`** has no concrete implementation — it only stores an API key.
- **`Stock::getSpecificReturn(Years holdingPeriod)`** doesn't yet model returns as a function of holding period/volatility; a `TODO` notes it should eventually apply some distribution (normal by default) rather than just re-returning `value_ - priceBought_`.
