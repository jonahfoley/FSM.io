#ifndef FSM_BUILDER_H
#define FSM_BUILDER_H

#include "FSM_elements.hpp"

#include "tl/expected.hpp"
#include "fmt/format.h"

#include <vector>
#include <string>
#include <ranges>
#include <algorithm>
#include <numeric>

namespace fsm
{   

    namespace views  = std::views;
    namespace ranges = std::ranges;

    // to implement the visitor pattern
    template <typename... Ts>
    struct Overload : Ts...
    {
        using Ts::operator()...;
    };
    template <class... Ts>
    Overload(Ts...) -> Overload<Ts...>;

    class FSMBuilder
    {
    public:
        FSMBuilder()
            : m_state_variables{},
              m_case_states{},
              m_state_count{0}
        {
            m_FSM_template = {};
            m_next_state_template = {};
            m_state_reg_template = {};
            m_state_template = {};
        }

        auto build(const std::vector<parser::FSMToken> &toks) -> std::string
        {

            auto m_state = [this](parser::FSMState s)
            {
                // update the state variables
                auto state = fmt::format("s{}", m_state_count);
                m_state_variables.push_back(state);

                // for the first state update the default sate
                if (m_state_count == 0)
                {
                    m_default_state = state;
                }

                // setup the outputs
                std::string outputs;
                for (std::string_view output : s.m_outputs)
                {   
                    outputs = fmt::format("\t{}{} = '1;\n", outputs, output);
                }

                // write the case
                auto case_state = fmt::format(
                    "{} : begin\n"
                    "{}\n"
                    "end" , 
                    state, 
                    outputs
                );
                m_case_states.push_back(case_state);

                m_state_count++;
            };

            auto m_transition = [this](parser::FSMTransition t)
            {
                auto x = t.m_decision.m_id;
            };

            auto m_token_printer = Overload{
                m_state,
                m_transition
            };

            for (const auto &tok : toks)
            {
                std::visit(m_token_printer, tok);
            }

            // assemble the output
            auto state_variables = fmt::format(
                "typedef enum {{\n"
                "\t{}\n"
                "}} state_t;\n\n" 
                "state_t present_state, next_state;",
                join_strings(m_state_variables, ",")
            );

            auto state_register = fmt::format(
                "always_ff @( posedge clk ) begin : sync\n"
                "if (reset)\n"
                "\tpresent_state <= {};\n"
                "else\n"
                "\tpresent_state <= next_state;\n"
                "end",
                m_default_state
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
                fmt::arg("cases", ""),
                fmt::arg("default_state", m_default_state)
            );

            auto res = fmt::format(
                "{state_variables}\n\n"
                "{state_register}\n\n"
                "{next_state}\n\n",
                fmt::arg("state_variables", state_variables),
                fmt::arg("state_register", state_register),
                fmt::arg("next_state", next_state_logic)
            );

            return res;
        }

        void print_state() 
        {
            std::for_each(m_state_variables.begin(), m_state_variables.end(), [](std::string_view s){ fmt::print("{}\n", s); });
            fmt::print("{}", m_default_state);
            std::for_each(m_case_states.begin(), m_case_states.end(), [](std::string_view s){ fmt::print("{}\n", s); });
        } 

    private:

        auto join_strings(auto& container, const std::string& delim) -> std::string
        {
            return std::accumulate(container.begin(), container.end(), std::string(), 
            [delim](const std::string& a, const std::string& b) -> std::string { 
                return a + (a.length() > 0 ? delim : "") + b; 
            });
        }
        
        // the actual elements of the FSM to be written out
        std::vector<std::string> m_state_variables;
        std::vector<std::string> m_case_states;
        std::string m_default_state;

        // templates -> these can probably be hard coded
        std::string m_FSM_template;
        std::string m_next_state_template;
        std::string m_state_reg_template;
        std::string m_state_template;

        int m_state_count;
    };

}

#endif