#ifndef MODEL_H
#define MODEL_H

#include "FSM_elements.hpp"
#include "tree.hpp"
#include "parser.hpp"
#include "type_aliases.hpp"

#include <string_view>
#include <optional>
#include <algorithm>
#include <ranges>
#include <vector>
#include <unordered_map>

namespace model
{
    [[nodiscard]]
    auto build_transition_matrix(
        const std::vector<parser::FSMState>& states,
        const std::vector<parser::FSMPredicate>& predicates,
        const std::vector<parser::FSMArrow>& arrows
    ) -> ::TransitionMatrix;

    [[nodiscard]]
    auto build_transition_tree_map(
        const std::vector<parser::FSMState>& states,
        const std::vector<parser::FSMPredicate>& predicates,
        const std::vector<std::vector<std::optional<bool>>>& transition_matrx
    ) -> ::StateTransitionMap;
}

#endif