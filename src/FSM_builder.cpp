#include "../include/FSM_builder.hpp"

#include "fmt/format.h"

namespace fsm {

    auto build_FSM(const std::vector<parser::FSMToken> toks) -> std::string
    {
        auto TypeOfElem = Overload {                                    
            [](parser::FSMState s) { return s.m_id; },
            [](parser::FSMDecision d) { return std::string("DECISION"); },
            [](parser::FSMArrow a) { return std::string("ARROW"); },
            [](parser::FSMTransition t) { 
                return fmt::format(
                    "TRANSITION for: {}\npredicate: {}\ntrue: {}, false {}", 
                    t.m_decision.m_id, t.m_decision.m_predicate, t.m_true_path.m_target, t.m_false_path.m_target); 
            },
            [](auto) { return std::string("other"); }
        };

        for (auto tok : toks) {
            fmt::print("{}\n", std::visit(TypeOfElem, tok));
        }

        return {};

    }

}