#include "../include/parser.hpp"
#include "../include/FSM_builder.hpp"

#include "fmt/format.h"
#include "tinyxml2.h"

#include <iostream>
#include <ranges>

auto main() -> int
{
    std::string_view dir{"../../resources/test.xml"};

    auto fsm_tokens =
        parser::extract_encoded_drawio(dir)
            .and_then(parser::base64_decode)
            .and_then(parser::inflate)
            .and_then(parser::url_decode)
            .and_then(parser::drawio_to_tokens)
            .or_else(parser::HandleParseError);
    
    auto res = fsm::build_FSM(fsm_tokens.value());
        
    return 0;
}
