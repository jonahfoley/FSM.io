#include "../include/parser.hpp"
#include "../include/model.hpp"
#include "../include/FSM_builder.hpp"

#include "fmt/format.h"

auto main() -> int
{
    std::string_view dir{"../../resources/test_2.drawio"};

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
    auto m = model::build_transition_matrix(s, p, a);
    
    // for the transition binary trees
    auto transition_trees = model::build_transition_tree(s, p, m);

    // build the output string
    fsm::FSMBuilder builder{std::move(s), std::move(transition_trees)};
    fmt::print("{}", builder.write());

    return 0;
}
