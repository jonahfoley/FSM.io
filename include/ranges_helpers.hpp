#ifndef RANGES_HELPERS_H
#define RANGES_HELPERS_H

#include <ranges>
#include <vector>
#include <numeric>
#include <algorithm>

#include "fmt/format.h" 

// from: https://stackoverflow.com/questions/58808030/range-view-to-stdvector
namespace utility
{
    namespace ranges = std::ranges;
    namespace views = std::views;
    
    // First go, just as a function taking a range and converting
    // to a vector only
    template <std::ranges::range R>
    constexpr auto to_vector(R &&r)
    {
        using elem_t = std::decay_t<std::ranges::range_value_t<R>>;
        return std::vector<elem_t>{r.begin(), r.end()};
    }

    // Second go, using ranges style piping
    namespace detail
    {

        // Type acts as a tag to find the correct operator| overload
        template <typename C>
        struct to_helper
        {
        };

        // This actually does the work
        template <typename Container, std::ranges::range R>
            requires std::convertible_to<std::ranges::range_value_t<R>, typename Container::value_type>
        Container operator|(R &&r, to_helper<Container>)
        {
            return Container{r.begin(), r.end()};
        }

    }

    // Couldn't find an concept for container, however a
    // container is a range, but not a view.
    template <std::ranges::range Container>
        requires(!std::ranges::view<Container>)
    auto to()
    {
        return detail::to_helper<Container>{};
    }
}

#endif