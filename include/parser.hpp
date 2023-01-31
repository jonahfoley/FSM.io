#ifndef PARSER_H
#define PARSER_H

#include <string_view>
#include <tl/expected.hpp>

namespace parser {

    enum class ParseError {
        InvalidChar
    }

    auto inflate_string(std::string_view compressed_str) -> tl::expected<std::string_view, ParseError>;
    auto base64_decode(std::string_view encoded_str) -> tl::expected<std::string_view, ParseError>;
    auto url_decode(std::string_view encoded_str) -> tl::expected<std::string_view, ParseError>;
}

#endif