#ifndef PARSER_H
#define PARSER_H

#include <string_view>
#include <optional>
#include <variant>
#include <ranges>

#include "tl/expected.hpp"
#include "tinyxml2.h"
#include "FSM_elements.hpp"

namespace parser
{

    enum class ParseError
    {
        InvalidDrawioFile,
        ExtractingDrawioString,
        URLDecodeError,
        Base64DecodeError,
        InflationError,
        DrawioToToken
    };

    void HandleParseError(const ParseError err);

    [[nodiscard]] auto extract_encoded_drawio(std::string_view file_name) -> tl::expected<std::string, ParseError>;

    [[nodiscard]] auto inflate(std::string_view str) -> tl::expected<std::string, ParseError>;

    [[nodiscard]] auto base64_decode(std::string_view encoded_str) -> tl::expected<std::string, ParseError>;

    [[nodiscard]] auto url_decode(std::string_view encoded_str) -> tl::expected<std::string, ParseError>;

    [[nodiscard]] auto drawio_to_tokens(std::string_view drawio_xml_str) -> tl::expected<std::vector<FSMToken>, ParseError>;
}

#endif