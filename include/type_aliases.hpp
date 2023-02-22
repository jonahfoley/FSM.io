#ifndef TYPE_ALIASES_H
#define TYPE_ALIASES_H
 
#include "utility.hpp"
#include "tree.hpp"
#include "FSM_elements.hpp"

#include <optional>
#include <vector>

using TokenTuple = std::tuple<std::vector<parser::FSMState>, std::vector<parser::FSMPredicate>, std::vector<parser::FSMArrow>>;

using TransitionTree    = utility::binary_tree<parser::FSMTransition>;
using TransitionNode    = utility::Node<parser::FSMTransition>;
using TransitionMatrix  = std::vector<std::vector<std::optional<bool>>>;

using StateTransitionMap = std::vector<std::pair<parser::FSMState, TransitionTree>>;

#endif