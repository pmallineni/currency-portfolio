#pragma once
#include <string>
#include <cstdint>


struct Currency
{
    const std::string_view name;
    const char* symbol;
    const std::string_view isoCode;
    const int unitPips;

    Currency(const Currency&) = delete;
    Currency& operator=(const Currency&) = delete;
};


template <const Currency& Instance> 
struct CurrencyTag
{
    static constexpr const Currency& instance = Instance;
};



class Rate
{
    private:
    // change both of these variables, if making a change to SCALE
    static const int scalePower {8};
    static const std::int64_t SCALE = 100000000; // 10^8
    std::int64_t value_;

    static constexpr std::int64_t sv_to_int(std::string_view sv)
    {
        std::int64_t val = 0;
        for (char c : sv)
        {
            if (c >= '0' && c <= '9') val = val * 10 + (c - '0');
        }
        return val;
    }

    explicit constexpr Rate(std::int64_t val) : value_(val) {}

    public:
     static constexpr Rate from_parts(std::int64_t rawRate)
    {
        return Rate(rawRate);
    }

    static constexpr Rate from_string(std::string_view str)
    {
        std::size_t decimalPoint = str.find('.');

        if(decimalPoint == std::string_view::npos)
            return Rate(sv_to_int(str) * Rate::SCALE);
        
        std::string_view whole = str.substr(0, decimalPoint);
        std::string_view frac = str.substr(decimalPoint + 1);

        std::int64_t wholeVal = sv_to_int(whole) * Rate::SCALE; 
        std::int64_t fracVal  = sv_to_int(frac.substr(0, Rate::scalePower));
        std::size_t digits = frac.size();

        while (digits < 8)
        {
            fracVal *= 10;
            digits++;
        }

        return Rate(wholeVal + fracVal);
    }

    struct detail
    {
        static int64_t getRaw(Rate r) { return r.value_;}
        static int64_t getScale() {return Rate::SCALE;}
    };
    
};

constexpr Rate operator"" _rate(const char* str, size_t len)
{
    return Rate::from_string(std::string_view(str, len));
}
