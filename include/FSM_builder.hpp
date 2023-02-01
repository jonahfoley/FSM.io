#ifndef FSM_BUILDER_H
#define FSM_BUILDER_H

#include "FSM_elements.hpp"

#include "tl/expected.hpp"

#include <vector>
#include <string>

namespace fsm {
    // to implement the visitor pattern
    template<typename ... Ts>                                                 
    struct Overload : Ts ... { 
        using Ts::operator() ...;
    };
    template<class... Ts> Overload(Ts...) -> Overload<Ts...>;

    [[nodiscard]]
    auto build_FSM(const std::vector<parser::FSMToken> toks) -> std::string;
}


#endif