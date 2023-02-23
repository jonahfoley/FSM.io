#ifndef FSM_BUILDER_H
#define FSM_BUILDER_H

#include "FSM_elements.hpp"
#include "tree.hpp"
#include "observer.hpp"
#include "utility.hpp"

#include <vector>
#include <string>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <cassert>
#include <unordered_map>

namespace fsm
{
    // namespace aliases
    namespace views  = std::views;
    namespace ranges = std::ranges;

    class FSMBuilder
    {
    public:
        FSMBuilder(StateTransitionMap& state_transition_map);

        // based on the vector of states and transition trees this builds the correctly
        // formatted output string of the corresponding systemverilog implementation
        auto build() -> void;
        
        // writes the systemverilog string to the output, rebuilding it if
        // it has detected an update to the states or transition trees
        auto write() -> std::string;
    
    private:
        // for a given state this writes it's case statement for the next state logic
        auto write_case_state(
            std::string_view state_name,
            const parser::FSMState& state, 
            const TransitionTree& transition_tree
        ) -> std::string;

        // for a given transition tree this recursively forms the if-else logic which gives the next state
        auto write_transition(const TransitionTree& transition_tree) -> std::string;
        auto write_transition_impl(const std::unique_ptr<TransitionNode>& node) -> std::string;
        
        // if these get modified, we need to update m_fsm_string, else we know
        // we can just output the previously computed version
        utility::Observed<StateTransitionMap> m_state_transition_map;

        // maps drawio id to s{i}
        std::unordered_map<std::string, std::string> m_id_state_map;
        
        // the formatted systemverilog version of the FSM
        std::string m_fsm_string;
    };
    
}

#endif