#include <iostream>
#include "../include/parser.hpp"
#include "fmt/format.h"
#include "tinyxml2.h"

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
    
    auto toks = parser::drawio_to_tokens(drawio_xml_str.value());

    fmt::print("Result: \n\n {}",  drawio_xml_str.value());

    return 0;
}
