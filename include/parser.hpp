#ifndef PARSER_H
#define PARSER_H

#include "../include/tree.hpp"

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
        DrawioToToken,
        DecisionPathError
    };

    void HandleParseError(const ParseError err);

    [[nodiscard]] 
    auto extract_encoded_drawio(std::string_view file_name) -> tl::expected<std::string, ParseError>;

    [[nodiscard]] 
    auto inflate(std::string_view str) -> tl::expected<std::string, ParseError>;

    [[nodiscard]] 
    auto base64_decode(std::string_view encoded_str) -> tl::expected<std::string, ParseError>;

    [[nodiscard]] 
    auto url_decode(std::string_view encoded_str) -> tl::expected<std::string, ParseError>;

    [[nodiscard]] 
    auto drawio_to_tokens(std::string_view drawio_xml_str) 
        -> tl::expected<std::tuple<
            std::vector<FSMState>,
            std::vector<FSMPredicate>,
            std::vector<FSMArrow>
            >, ParseError>;

    [[nodiscard]]
    auto build_transition_matrix(
        std::vector<FSMState>& states,
        std::vector<FSMPredicate>& predicates,
        std::vector<FSMArrow>& arrows
    ) -> std::vector<std::vector<std::optional<bool>>>;

    [[nodiscard]]
    auto build_decisions(
        std::vector<FSMArrow>& arrows,
        std::vector<FSMPredicate>& predicates
    ) -> tl::expected<std::vector<FSMDecision>, ParseError>;
    
    [[nodiscard]]
    auto build_transition_tree(
        std::vector<FSMState>& states,
        std::vector<FSMPredicate>& predicates,
        std::vector<std::vector<std::optional<bool>>>& transition_matrx
    ) -> std::vector<utility::binary_tree<FSMTransition>>;

    [[nodiscard]]
    auto build_transition_tree(
        std::vector<FSMState>& states,
        std::vector<FSMArrow>& arrows,
        std::vector<FSMDecision>& decisions
    ) -> std::vector<utility::binary_tree<FSMTransition>>;
}

#endif