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
    // namespace aliases
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
            const std::vector<parser::FSMState>& states,
            std::vector<TransitionTree>& transition_trees
        )
            : m_state{states, std::move(transition_trees)}
        {
            build();
        }

        // based on the vector of states and transition trees this builds the correctly
        // formatted output string of the corresponding systemverilog implementation
        auto build() -> void
        {
            // there is one transition tree for every state
            assert(m_state.m_states.value().size() == m_state.m_transition_trees.value().size());

            // first parse - generate the drawio_id -> state name mapping
            for (unsigned i = 0; i < m_state.m_states.value().size(); ++i)
            {
                std::string state_name = fmt::format("s{}", i);
                m_state_variables.push_back(state_name);
                m_id_state_map[m_state.m_states.value()[i].m_id] = state_name;

                if (i == 0)
                {
                    m_default_state = state_name;
                }
            }

            // second parse - build each of the case statements
            for (const auto& tree : m_state.m_transition_trees.value())
            {
                auto current_state = ranges::find_if(
                    m_state.m_states.value(),
                    [&tree](const auto& state){ return state.m_id == tree.m_root->m_value.m_id; }
                );

                if (current_state != m_state.m_states.value().end())
                {
                    auto default_values = write_state_default_outputs(*current_state);
                    m_default_outputs.push_back(default_values);

                    auto case_state = write_case_state(m_id_state_map[current_state->m_id], *current_state, tree);
                    m_case_states.push_back(case_state);

                    auto output_signals = write_output_signals(*current_state);
                    m_output_signals.push_back(output_signals);
                }

                // write the input signal (predicate of transition tree)
                auto input_signal = write_input_signals(tree);
                m_input_signals.push_back(input_signal);
            }

            // write the headers
            auto header = fmt::format(
                "module fsm (\n"
                "  input logic clk, reset,\n"
                "  input logic {},\n"
                "  output logic {}\n"
                ");",
                utility::join_strings(m_input_signals, ","),
                utility::join_strings(m_output_signals, ",")
            );

            // for the declaration of states
            auto states_declaration = fmt::format(
                "typedef enum {{\n"
                "  {}\n"
                "}} state_t;\n\n"
                "state_t present_state, next_state;",
                utility::join_strings(m_state_variables, ",")
            );

            // the synchronous register of current state
            auto state_register = fmt::format(
                "always_ff @( posedge clk ) begin : sync\n"
                "  if (reset)\n"
                "    present_state <= {};\n"
                "  else\n"
                "    present_state <= next_state;\n"
                "end",
                m_default_state
            );

            // the comb logic
            auto next_state_logic = fmt::format(
                "always_comb begin : comb\n"
                "  {}\n"
                "  next_state = present_state;\n"
                "  case (present_state)\n"
                "{}\n"
                "    default : begin\n"
                "      next_state = {};\n"
                "    end\n"
                "  endcase\n"
                "end",
                utility::join_strings(m_default_outputs, "\n"),
                utility::join_strings(m_case_states, "\n"),
                m_default_state
            );

            m_fsm_string = fmt::format(
                "{}\n\n"
                "{}\n\n"
                "{}\n\n"
                "{}\n\n"
                "endmodule",
                header,
                states_declaration,
                state_register,
                next_state_logic
            );
        }
        
        // writes the systemverilog string to the output, rebuilding it if
        // it has detected an update to the states or transition trees
        auto write() -> std::string
        {
           if (
                utility::any_of_modified(m_state.m_states,m_state.m_transition_trees) 
            )
            {
                build();
            } 
            return m_fsm_string;
        }
    
    private:
        // get the input signals (the predicates of the decision nodes) by recursively
        // traversion the transition tree and extracting the predicates at each node
        auto write_input_signals(
            const TransitionTree& tree
        ) -> std::string
        {
            std::vector<std::string> predicates;
            auto& root = tree.m_root;
            write_input_signals_impl(root, predicates);
            return utility::join_strings(predicates,", ");
        }

        // performs the traversal on the transition tree
        auto write_input_signals_impl(
            const std::unique_ptr<TransitionNode>& node,
            std::vector<std::string>& predicates
        ) -> void
        {
            if (node != nullptr)
            {
                if(node->m_value.m_predicate.has_value())
                {
                    predicates.push_back(node->m_value.m_predicate.value());
                }
                write_input_signals_impl(node->m_left, predicates);
                write_input_signals_impl(node->m_right, predicates);
            }
            else 
            {
                return;
            }
        }

        // joins together, into a comma-separated string, all the output signals for a given state
        auto write_output_signals(
            const parser::FSMState& state
        ) -> std::string
        {   
            return utility::join_strings(state.m_outputs,", ");
        }

        // joins together, in a comma-serparated string, all of the output signals set to their default value of 0
        auto write_state_default_outputs(
            const parser::FSMState& state
        ) -> std::string
        {
            return utility::join_strings(
                state.m_outputs | views::transform([](const auto& s){ return s + " = '0;"; }),
                "\n  "
            );
        }

        // for a given state this writes it's case statement for the next state logic
        auto write_case_state(
            std::string_view state_name,
            const parser::FSMState& state, 
            const TransitionTree& transition_tree
        ) -> std::string
        {
            if (state.m_outputs.size())
            {
                auto outputs = state.m_outputs
                    | views::transform([](const auto& s){ return s + " = '1;"; })
                    | utility::to<std::vector<std::string>>();

                return fmt::format(
                    "    {} : begin\n"
                    "      {}\n"
                    "{}\n"
                    "    end",
                    state_name,
                    utility::join_strings(outputs, "\n      "),
                    write_transition(transition_tree)
                );
            }
            else 
            {
                return fmt::format(
                    "    {} : begin\n"
                    "      {}\n"
                    "    end",
                    state_name,
                    write_transition(transition_tree)
                );
            }
        }

        // for a given transition tree this recursively forms the if-else logic which gives the next state
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
                return std::string{};
            }
        }

        // performs the recursion on the transition tree to extract the correctly formatted if-else logic
        auto write_transition_impl(const std::unique_ptr<TransitionNode>& node) -> std::string
        {
            // never should have just m_right be non-null 
            assert(node->m_left != nullptr && node->m_right == nullptr);

            // once we are at the leaf nodes we can print the transition
            if(node->m_left == nullptr && node->m_right == nullptr)
            {
                return fmt::format("next_state = {};",  m_id_state_map[node->m_value.m_id]);
            }
            else if(node->m_left != nullptr && node->m_right != nullptr)
            {   
                // if there are children but not a decision block, then must be an error
                assert(node.m_decision.value());
                return fmt::format(
                    "      if({}) begin\n"
                    "        {}\n"
                    "      end else begin\n"
                    "        {}\n"
                    "      end\n",
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
            // wont ever reach due to the assertion
            else
            {
                return std::string{};
            }
        }
        
        // if these get modified, we need to update m_fsm_string, else we know
        // we can just output the previously computed version
        struct State
        {
            Observed<std::vector<parser::FSMState>> m_states;
            Observed<std::vector<TransitionTree>> m_transition_trees;
        } m_state;
        
        // formatted data
        std::vector<std::string> m_state_variables;
        std::vector<std::string> m_case_states;
        std::vector<std::string> m_default_outputs;
        std::vector<std::string> m_output_signals;
        std::vector<std::string> m_input_signals;
        std::string m_default_state;

        // maps drawio id to s{i}
        std::unordered_map<std::string, std::string> m_id_state_map;
        
        // the formatted systemverilog version of the FSM
        std::string m_fsm_string;
    };
    
}

#endif