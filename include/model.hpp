#ifndef MODEL_H
#define MODEL_H

#include "../include/FSM_elements.hpp"
#include "../include/tree.hpp"
#include "../include/parser.hpp"

#include <string_view>
#include <optional>
#include <algorithm>
#include <ranges>
#include <vector>

namespace model
{

    [[nodiscard]]
    auto build_transition_matrix(
        const std::vector<parser::FSMState>& states,
        const std::vector<parser::FSMPredicate>& predicates,
        const std::vector<parser::FSMArrow>& arrows
    ) -> std::vector<std::vector<std::optional<bool>>>;

    [[nodiscard]]
    auto build_transition_tree(
        const std::vector<parser::FSMState>& states,
        const std::vector<parser::FSMPredicate>& predicates,
        const std::vector<std::vector<std::optional<bool>>>& transition_matrx
    ) -> std::vector<utility::binary_tree<parser::FSMTransition>>;
}

#endif