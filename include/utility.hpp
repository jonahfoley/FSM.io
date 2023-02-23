#ifndef UTILITY_H
#define UTILITY_H

#include <string>
#include <string_view>
#include <numeric> 
#include <ranges>

#include <fmt/format.h>

namespace utility
{
    namespace ranges = std::ranges;
    namespace views = std::views;

    auto join_non_empty_strings(auto&& container, std::string_view delim) -> std::string
    {
        return fmt::format("{}", fmt::join(
                container | views::filter([](std::string_view s){ return !s.empty(); }), //filter the length zero elements
                delim
            )
        );
    }
}

#endif