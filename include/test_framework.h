#pragma once
// Minimal, dependency-free test harness.
// Unlike assert(), a failed CHECK prints and keeps going, so one bad
// assumption doesn't hide every other test result in the same run.

#include <iostream>
#include <string>
#include <functional>
#include <vector>

namespace TestFramework
{
    inline int g_passed = 0;
    inline int g_failed = 0;

    inline void check(bool condition, const std::string& description, const char* file, int line)
    {
        if (condition)
        {
            ++g_passed;
        }
        else
        {
            ++g_failed;
            std::cout << "  [FAIL] " << description << "  (" << file << ':' << line << ")\n";
        }
    }

    template <typename Func>
    inline void checkThrows(Func&& func, const std::string& description, const char* file, int line)
    {
        bool threw = false;
        try { func(); }
        catch (...) { threw = true; }
        check(threw, description + " (expected an exception)", file, line);
    }

    template <typename Func>
    inline void checkNoThrow(Func&& func, const std::string& description, const char* file, int line)
    {
        bool threw = false;
        try { func(); }
        catch (...) { threw = true; }
        check(!threw, description + " (expected no exception)", file, line);
    }

    inline void runSection(const std::string& name, const std::function<void()>& body)
    {
        std::cout << name << '\n';
        body();
    }

    inline int summarize()
    {
        std::cout << "\n" << g_passed << " passed, " << g_failed << " failed.\n";
        return g_failed == 0 ? 0 : 1;
    }
}

#define CHECK(cond) TestFramework::check((cond), #cond, __FILE__, __LINE__)
#define CHECK_MSG(cond, msg) TestFramework::check((cond), msg, __FILE__, __LINE__)
#define CHECK_THROWS(expr) TestFramework::checkThrows([&](){ expr; }, #expr, __FILE__, __LINE__)
#define CHECK_NO_THROW(expr) TestFramework::checkNoThrow([&](){ expr; }, #expr, __FILE__, __LINE__)
