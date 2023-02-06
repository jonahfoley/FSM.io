#include "../include/parser.hpp"
#include "../include/FSM_builder.hpp"

#include "fmt/format.h"
#include "tinyxml2.h"

#include <iostream>
#include <ranges>

auto main() -> int
{
    std::string_view dir{"../../resources/test.xml"};

    // turn the encoded XML into tokens
    auto token_tuple =
        parser::extract_encoded_drawio(dir)
            .and_then(parser::base64_decode)
            .and_then(parser::inflate)
            .and_then(parser::url_decode)
            .and_then(parser::drawio_to_tokens)
            .or_else(parser::HandleParseError);

    // break down the tuple into (s)tates, (p)redicates, and (a)rrows
    auto& [s, p, a] = token_tuple.value();

    // get the decisions
    auto m = build_transition_matrix(s, p, a);

    // for the transition binary trees
    auto transition_trees = build_transition_tree(s, p, m);

    auto builder = fsm::FSMBuilder(s, transition_trees);
    fmt::print("\n ------------------------------------- \n {}", builder.write());

    return 0;
}
