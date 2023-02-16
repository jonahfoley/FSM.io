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

    // type aliases
    using TransitionTree = utility::binary_tree<parser::FSMTransition>;
    using TransitionNode = utility::Node<parser::FSMTransition>;

    class FSMBuilder
    {
    public:
        FSMBuilder(
            std::vector<parser::FSMState>&& states,
            std::vector<TransitionTree>&& transition_trees 
        );

        // based on the vector of states and transition trees this builds the correctly
        // formatted output string of the corresponding systemverilog implementation
        auto build() -> void;
        
        // writes the systemverilog string to the output, rebuilding it if
        // it has detected an update to the states or transition trees
        auto write() -> std::string;
    
    private:
        // get the input signals (the predicates of the decision nodes) by recursively
        // traversion the transition tree and extracting the predicates at each node
        auto write_input_signals(const TransitionTree& tree) -> std::string;

        // joins together, into a comma-separated string, all the output signals for a given state
        auto write_output_signals(const parser::FSMState& state) -> std::string;

        // joins together, in a comma-serparated string, all of the output signals set to their default value of 0
        auto write_state_default_outputs(const parser::FSMState& state) -> std::string;

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
        struct State
        {
            utility::Observed<std::vector<parser::FSMState>> m_states;
            utility::Observed<std::vector<TransitionTree>> m_transition_trees;
        } m_state;

        // maps drawio id to s{i}
        std::unordered_map<std::string, std::string> m_id_state_map;
        
        // the formatted systemverilog version of the FSM
        std::string m_fsm_string;
    };
    
}

#endif