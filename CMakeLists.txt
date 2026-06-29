cmake_minimum_required(VERSION 3.23)

project(stockstuff LANGUAGES CXX)

set (CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # For GCC/Clang: Use DWARF version 4 instead of 5, which is much more stable with CodeLLDB
    add_compile_options(-gdwarf-4)
    
    # OR if it keeps crashing, limit debug info to line tables (stops variable parsing crashes entirely)
    # add_compile_options(-g1)
endif()

include_directories(include)

add_executable(Executable
    src/main.cpp
    src/portfolio_views.cpp
    src/portfolio.cpp
    src/currency_conversions.cpp
    )

