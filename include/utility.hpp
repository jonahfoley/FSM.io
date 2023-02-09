#ifndef UTILITY_H
#define UTILITY_H

#include <string>
#include <string_view>
#include <numeric>

#include "fmt/format.h" 

// from: https://stackoverflow.com/questions/58808030/range-view-to-stdvector
namespace utility
{
    auto join_strings(const auto& container, const std::string& delim) -> std::string
    {
        return std::accumulate(container.begin(), container.end(), std::string(), 
        [delim](const std::string& a, const std::string& b) -> std::string { 
            return a + (a.length() > 0 ? delim : "") + b; 
        });
    }
}

#endif