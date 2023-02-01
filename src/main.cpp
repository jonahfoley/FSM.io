#include "../include/parser.hpp"
#include "../include/FSM_builder.hpp"

#include "fmt/format.h"
#include "tinyxml2.h"

#include <iostream>
#include <ranges>

auto main() -> int
{
    std::string_view dir{"../../resources/test.xml"};

    auto drawio_xml_str =
        parser::extract_encoded_drawio(dir)
            .and_then(parser::base64_decode)
            .and_then(parser::inflate)
            .and_then(parser::url_decode)
            .or_else(parser::HandleParseError);
    
    auto fsm_str = 
        parser::drawio_to_tokens(drawio_xml_str.value())
            .or_else([](auto&&){ throw std::runtime_error("dang");})
            .map(fsm::build_FSM);
        
    return 0;
}
