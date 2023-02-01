#ifndef PARSER_H
#define PARSER_H

#include <string_view>
#include "tl/expected.hpp"
#include <type_traits>

namespace parser {

    enum class ParseError {
        URLDecodeError,
        Base64DecodeError,
        InflationError
    };

    auto inflate(std::string_view str) -> tl::expected<std::string, ParseError>;
    auto base64_decode(std::string_view encoded_str) -> tl::expected<std::string, ParseError>;
    auto url_decode(std::string_view encoded_str) -> tl::expected<std::string, ParseError>;
};

#endif