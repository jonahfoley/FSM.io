#ifndef TRANSITION_MATRIX_H
#define TRANSITION_MATRIX_H

#include "FSM_elements.hpp"

#include <string_view>
#include <optional>
#include <algorithm>
#include <ranges>
#include <vector>
#include <unordered_map>

#include "fmt/format.h"

namespace model
{
    class TransitionMatrix
    {
    public:
        using Connections_t = std::vector<std::vector<std::optional<bool>>>;

        TransitionMatrix();
        TransitionMatrix(
            const States_t &states,
            const Arrows_t &arrows,
            const Predicates_t &predicates
        );

        auto operator()(unsigned row, unsigned col) -> std::optional<bool> &;
        auto operator()(unsigned row, unsigned col) const -> std::optional<bool>;

        auto rank() -> unsigned;
        auto rank() const -> unsigned;

        auto print() -> void;
        auto print() const -> void;

        auto set(const unsigned row, const unsigned col, const bool value) -> void;

    private:
        auto populate_connection_matrix(
            const States_t& states, 
            const Arrows_t& arrows, 
            const Predicates_t& predicates
        ) -> void;

        Connections_t m_connections;
        const unsigned m_rank;
    };

    [[nodiscard]] 
    auto build_transition_tree_map(
        const States_t& states,
        const Predicates_t& predicates,
        const TransitionMatrix& transition_matrix
    ) -> StateTransitionMap;
}

#endif