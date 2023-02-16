#include "../include/FSM_builder.hpp"
#include "../include/ranges_helpers.hpp"

#include <ranges>
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace fsm 
{

    using namespace ::utility;

    FSMBuilder::FSMBuilder(
        std::vector<parser::FSMState>&& states,
        std::vector<TransitionTree>&& transition_trees
    )
        : m_state{std::move(states), std::move(transition_trees)}
    {
        build();
    }
    
    static
    auto indent(std::string_view multi_line_str, unsigned indent_level) -> std::string
    {        
        std::string indent(indent_level * 2, ' ');
        return fmt::format(
            "{}{}",
            indent,
            fmt::join(
                multi_line_str 
                    | views::split('\n')
                    | views::transform([](auto r) { 
                        return std::string_view(r.begin(), r.end()); 
                    }),
                "\n"+indent
            )
        );
    }

    auto FSMBuilder::build() -> void
    {
        // there is one transition tree for every state
        assert(m_state.m_states.value().size() == m_state.m_transition_trees.value().size());

        std::vector<std::string> state_variables, case_states, default_outputs, output_signals, input_signals;
        std::string default_state;

        // first parse - generate the (drawio id) -> (state name) mapping
        for (unsigned i = 0; i < m_state.m_states.value().size(); ++i)
        {
            std::string state_name = fmt::format("s{}", i);
            state_variables.push_back(state_name);
            m_id_state_map[m_state.m_states.value()[i].m_id] = state_name;

            if (i == 0)
            {
                default_state = state_name;
            }
        }
        // second parse - build each of the case statements
        for (const auto& tree : m_state.m_transition_trees.value())
        {
            auto current_state = ranges::find_if(
                m_state.m_states.value(),
                [root_id = tree.m_root->m_value.m_id](const auto& state){ return state.m_id == root_id; }
            );

            if (current_state != m_state.m_states.value().end())
            {
                default_outputs.push_back(write_state_default_outputs(*current_state));
                case_states.push_back(write_case_state(m_id_state_map[current_state->m_id], *current_state, tree));
                output_signals.push_back(write_output_signals(*current_state));
            }
            // write the input signal (predicate of transition tree)
            input_signals.push_back(write_input_signals(tree));
        }

        // write the headers
        auto header = fmt::format(
            "module fsm (\n"
            "  input logic clk, reset,\n"
            "  input logic {},\n"
            "  output logic {}\n"
            ");",
            join_strings(input_signals, ","),
            join_strings(output_signals, ",")
        );

        // for the declaration of states
        auto states_declaration = fmt::format(
            "typedef enum {{\n"
            "  {}\n"
            "}} state_t;\n\n"
            "state_t present_state, next_state;",
            join_strings(state_variables, ",")
        );

        // the synchronous register of current state
        auto state_register = fmt::format(
            "always_ff @( posedge clk ) begin : sync\n"
            "  if (reset)\n"
            "    present_state <= {};\n"
            "  else\n"
            "    present_state <= next_state;\n"
            "end",
            default_state
        );

        // the comb logic
        auto next_state_logic = fmt::format(
            "always_comb begin : comb\n"
            "{}\n"
            "  next_state = present_state;\n"
            "  case (present_state)\n"
            "{}\n"
            "    default : begin\n"
            "      next_state = {};\n"
            "    end\n"
            "  endcase\n"
            "end",
            indent(join_strings(default_outputs, "\n"), 1),
            indent(join_strings(case_states, "\n"), 2),
            default_state
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

    auto FSMBuilder::write() -> std::string
    {
        if (any_of_modified(m_state.m_states,m_state.m_transition_trees))
        {
            build();
        } 
        return m_fsm_string;
    }

    static
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

    auto FSMBuilder::write_input_signals(const TransitionTree& tree) -> std::string
    {
        std::vector<std::string> predicates;
        auto& root = tree.m_root;
        write_input_signals_impl(root, predicates);
        return join_strings(predicates,", ");
    }

    auto FSMBuilder::write_output_signals(const parser::FSMState& state) -> std::string
    {   
        return join_strings(state.m_outputs,", ");
    }

    auto FSMBuilder::write_state_default_outputs(const parser::FSMState& state ) -> std::string
    {
        return join_strings(
            state.m_outputs | views::transform([](const auto& s){ return s + " = '0;"; }),
            "\n"
        );
    }

    auto FSMBuilder::write_case_state(
        std::string_view state_name,
        const parser::FSMState& state, 
        const TransitionTree& transition_tree
    ) -> std::string
    {
        if (state.m_outputs.size())
        {
            auto outputs = state.m_outputs
                | views::transform([](const auto& s){ return s + " = '1;"; })
                | to<std::vector<std::string>>();

            return fmt::format(
                "{} : begin\n"
                "{}\n"
                "{}\n"
                "end",
                state_name,
                indent(join_strings(outputs, "\n"), 1),
                indent(write_transition(transition_tree), 1)
            );
        }
        else 
        {
            return fmt::format(
                "{} : begin\n"
                "{}\n"
                "end",
                state_name,
                indent(write_transition(transition_tree), 1)
            );
        }
    }

    auto FSMBuilder::write_transition_impl(const std::unique_ptr<TransitionNode>& node) -> std::string
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
                "if({}) begin\n"
                "{}\n"
                "end else begin\n"
                "{}\n"
                "end\n",
                node->m_value.m_predicate.value(),
                indent(write_transition_impl(node->m_left), 1),
                indent(write_transition_impl(node->m_right),1)
            );
        }
        // the start of the tree
        else if (node->m_left != nullptr && node->m_right == nullptr)
        {
            return write_transition_impl(node->m_left);
        }
        else
        {
            return std::string{};
        }
    }

    auto FSMBuilder::write_transition(const TransitionTree& transition_tree) -> std::string
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
}