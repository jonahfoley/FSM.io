#include "../include/FSM_builder.hpp"

#include "fmt/format.h"

namespace fsm {

    auto build_FSM(const std::vector<parser::FSMToken> toks) -> std::string
    {
        auto TypeOfElem = Overload {                                    
            [](parser::FSMState) { return "STATE"; },
            [](parser::FSMDecision) { return "DECISION"; },
            [](parser::FSMArrow) { return "ARROW"; },
            [](auto) { return "unknown type"; }
        };

        for (auto tok : toks) {
            fmt::print("{}\n", std::visit(TypeOfElem, tok));
        }

        return {};

    }

}