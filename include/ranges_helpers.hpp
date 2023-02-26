#ifndef RANGES_HELPERS_H
#define RANGES_HELPERS_H

#include <ranges>
#include <vector>
#include <numeric>
#include <algorithm>

#include "tl/expected.hpp"

// from: https://stackoverflow.com/questions/58808030/range-view-to-stdvector
namespace utility
{
    namespace ranges = std::ranges;
    namespace views = std::views;

    // Second go, using ranges style piping
    namespace detail
    {
        // Type acts as a tag to find the correct operator| overload
        template <typename C>
        struct to_helper {};

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

    template<class>
    constexpr bool is_expected = false;

    template<class T, class E>
    constexpr bool is_expected<tl::expected<T, E>> = true;

    template<ranges::input_range R>
    requires is_expected<ranges::range_value_t<R>>
    auto to_expected(R&& r) {
        using expected_type = ranges::range_value_t<R>;
        using value_type = expected_type::value_type;
        using error_type = expected_type::error_type;
        using return_type = tl::expected<std::vector<value_type>, error_type>;

        auto values = r
            | views::take_while([](const auto& e) { return e.has_value(); })
            | views::transform([](const auto& e) { return e.value(); });

        // for simplicity, only use vector to store the result
        std::vector<value_type> v;
        auto [it, out] = ranges::copy(values, std::back_inserter(v));
        if (it.base() == r.end()) // all success
            return return_type(std::move(v));
        // return the first error
        return return_type(tl::unexpect, (*it.base()).error());
    };
}

#endif