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

        std::vector<std::string> state_variables, case_states;
        std::string default_state;

        // first parse - generate the (drawio id) -> (state name) mapping
        for (unsigned i = 0; i < m_state.m_states.value().size(); ++i)
        {
            std::string state_name = m_state.m_states.value()[i].m_state_name.value_or(fmt::format("s{}", i));
            state_variables.push_back(state_name);
            m_id_state_map[m_state.m_states.value()[i].m_id] = state_name;

            if (i == 0)
            {
                default_state = state_name;
            }
        }
        
        // second parse - make the case statements and then extract unique inputs and outputs from the states/trees
        std::vector<std::string> outputs, inputs;
        for (const auto& state : m_state.m_states.value())
        {
            const auto current_tree = ranges::find_if(
                m_state.m_transition_trees.value(),
                [&state](const auto& tree){ return state.m_id == tree.m_root->m_value.m_id; }
            );
            case_states.push_back(
                write_case_state(m_id_state_map[state.m_id], state, *current_tree)
            );
            // copy over the unique inputs and outputs
            ranges::copy_if(
                input_signals(*current_tree),
                std::back_inserter(inputs),
                [&inputs](std::string_view s){ return ranges::find(inputs, s) == inputs.end(); }
            );
            ranges::copy_if(
                state.m_outputs,
                std::back_inserter(outputs),
                [&outputs](std::string_view s){ return ranges::find(outputs, s) == outputs.end(); }
            );
        }

        // write the headers
        auto header = fmt::format(
            "module fsm (\n"
            "  input logic clk, reset,\n"
            "  input logic {},\n"
            "  output logic {}\n"
            ");",
            join_strings(inputs, ","),
            join_strings(outputs, ",")
        );

        // for the declaration of states
        auto states_declaration = fmt::format(
            "typedef enum {{\n"
            "  {}\n"
            "}} state_t;\n\n"
            "state_t present_state, next_state;",
            join_strings(state_variables, ", ")
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
            indent(
                join_strings(outputs | views::transform([](std::string_view s){ return std::string(s) + " = '0;"; }), "\n"),
                1
            ),
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
        // use the cached string if the states and transition trees have not been modified
        if (any_of_modified(m_state.m_states, m_state.m_transition_trees))
        {
            build();
        } 
        return m_fsm_string;
    }

    static
    auto input_signals_impl(
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
            input_signals_impl(node->m_left, predicates);
            input_signals_impl(node->m_right, predicates);
        }
        else 
        {
            return;
        }
    }

    auto FSMBuilder::input_signals(const TransitionTree& tree) -> std::vector<std::string>
    {
        std::vector<std::string> predicates;
        auto& root = tree.m_root;
        input_signals_impl(root, predicates);
        return predicates;
    }

    auto FSMBuilder::write_case_state(
        std::string_view state_name,
        const parser::FSMState& state, 
        const TransitionTree& transition_tree
    ) -> std::string
    {
        if (!state.m_outputs.empty())
        {
            auto outputs = state.m_outputs
                | views::transform([](std::string_view s){ return std::string(s) + " = '1;"; });

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
            assert(node->m_value.m_predicate.has_value());
            return fmt::format(
                "if({}) begin\n"
                "{}\n"
                "end else begin\n"
                "{}\n"
                "end",
                node->m_value.m_predicate.value(),
                indent(write_transition_impl(node->m_left),1),
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