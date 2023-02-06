#ifndef FSM_BUILDER_H
#define FSM_BUILDER_H

#include "FSM_elements.hpp"
#include "utility.hpp"
#include "tree.hpp"
#include "observer.hpp"

#include "tl/expected.hpp"
#include "fmt/format.h"

#include <vector>
#include <string>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <cassert>

namespace utility
{
    // to implement the visitor pattern
    template <typename... Ts>
    struct Overload : Ts...
    {
        using Ts::operator()...;
    };
    template <class... Ts>
    Overload(Ts...) -> Overload<Ts...>;
}

namespace fsm
{
    namespace views  = std::views;
    namespace ranges = std::ranges;

    // type aliases
    using TransitionTree = utility::binary_tree<parser::FSMTransition>;
    using TransitionNode = utility::Node<parser::FSMTransition>;

    using namespace ::utility;

    class FSMBuilder
    {
    public:
        FSMBuilder(
            const std::vector<parser::FSMState> &states,
            const std::vector<TransitionTree>& transition_trees
        )
            : m_state{}
        {
            build(states, transition_trees);
            write();
        }

        auto build(
            const std::vector<parser::FSMState> &states,
            const std::vector<TransitionTree>& transition_trees
        ) -> void
        {
            assert(states.size() == transition_trees.size());

            for(unsigned state_count = 0; state_count < states.size(); ++state_count)
            {
                // update the state variables
                std::string state_name = fmt::format("s{}", state_count);
                m_state.m_state_variables.call<void, const std::string &>(&std::vector<std::string>::push_back, state_name);
                m_id_state_map[state_name] = states[state_count].m_id;

                // for the first state update the default sate
                if (state_count == 0)
                {
                    m_state.m_default_state.write(state_name);
                }

                // write the case states
                auto case_state = write_case_state(state_name, states[state_count], transition_trees[state_count]);
                m_state.m_case_states.call<void, const std::string &>(&std::vector<std::string>::push_back, case_state);
            }
        }
        
        auto write() -> std::string
        {
            // assemble the output
            if (
                utility::any_of_modified(
                    m_state.m_case_states,
                    m_state.m_default_state,
                    m_state.m_state_variables
                )
            )
            {
                auto state_variables = fmt::format(
                    "typedef enum {{\n"
                    "\t{}\n"
                    "}} state_t;\n\n"
                    "state_t present_state, next_state;",
                    utility::join_strings(m_state.m_state_variables.value(), ",")
                );

                auto state_register = fmt::format(
                    "always_ff @( posedge clk ) begin : sync\n"
                    "if (reset)\n"
                    "\tpresent_state <= {};\n"
                    "else\n"
                    "\tpresent_state <= next_state;\n"
                    "end",
                    m_state.m_default_state.value()
                );

                auto next_state_logic = fmt::format(
                    "always_comb begin : comb\n"
                    "\t{outputs_defaults}\n"
                    "\tnext_state = present_state;\n"
                    "\tcase (present_state)\n"
                    "\t\t{cases}\n"
                    "\t\tdefault : begin\n"
                    "\t\t\tnext_state = {default_state};\n"
                    "\t\tend\n"
                    "\tendcase\n"
                    "end",
                    fmt::arg("outputs_defaults", ""),
                    fmt::arg("cases", utility::join_strings(m_state.m_case_states.value(), "\n")),
                    fmt::arg("default_state", m_state.m_default_state.value())
                );

                m_fsm_string = fmt::format(
                    "{state_variables}\n\n"
                    "{state_register}\n\n"
                    "{next_state}\n\n",
                    fmt::arg("state_variables", state_variables),
                    fmt::arg("state_register", state_register),
                    fmt::arg("next_state", next_state_logic)
                );
            }
            return m_fsm_string;
        }
    
    private:
        auto write_case_state(
            std::string_view state_name,
            const parser::FSMState& state, 
            const TransitionTree& transition_tree
        ) -> std::string
        {
            std::string case_state{};
            if (state.m_outputs.size())
            {
                case_state = fmt::format(
                    "{} : begin\n"
                    "{}\n"
                    "{}\n"
                    "end",
                    state_name,
                    join_strings(state.m_outputs,"=1;\n"), // TODO: this is wrong
                    write_transition(transition_tree)
                );
            }
            else 
            {
                case_state = fmt::format(
                    "{} : begin\n"
                    "{}\n"
                    "end",
                    state_name,
                    write_transition(transition_tree)
                );
            }
            return case_state;
        }

        auto write_transition(const TransitionTree& transition_tree) -> std::string
        {
            const auto& root = transition_tree.m_root;
            if (root != nullptr)
            {
                return write_transition_impl(root);
            }
            // an empty circuit
            else 
            {
                return std::string{"empty circuit"};
            }
        }

        auto write_transition_impl(const std::unique_ptr<TransitionNode>& node) -> std::string
        {
            // never should have just m_right be non-null 
            assert(node->m_left != nullptr && node->m_right == nullptr);

            // once se are at the leaf nodes we can print the transition
            if(node->m_left == nullptr && node->m_right == nullptr)
            {
                return fmt::format("next_state = {};", m_id_state_map[node->m_value.m_id]);
            }
            // if both are not-null then both will have values (full binary tree)
            else if(node->m_left != nullptr && node->m_right != nullptr)
            {   
                // if there are children but not a decision block, then must be an error
                assert(node.m_decision.value());
                return fmt::format(
                    "if({}) begin\n"
                    "\t{}\n"
                    "end else begin\n"
                    "\t{}\n"
                    "end\n",
                    node->m_value.m_predicate.value(),
                    write_transition_impl(node->m_left),
                    write_transition_impl(node->m_right)
                );
            }
            // the start of the tree
            else if (node->m_left != nullptr && node->m_right == nullptr)
            {
                return write_transition_impl(node->m_left);
            }
            // shouldnt ever reach!
            else
            {
                return "";
            }
        }
        
        //- We calculate the initial state in the ctor based on the provided FSMStates and FSMTransitions
        //  and then write to the fsm_string variable
        //- if we then call write() again, and all of the state variables are yet to be changed then we
        //  can return the cached fsm_string
        //- if the state has been modified, then we re-write the string
        
        struct State
        {
            Observed<std::vector<std::string>> m_state_variables;
            Observed<std::vector<std::string>> m_case_states;
            Observed<std::string> m_default_state;
        };
        
        std::unordered_map<std::string, std::string> m_id_state_map;

        State m_state;
        std::string m_fsm_string;
    };
    
}

#endif