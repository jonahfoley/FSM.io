#ifndef FSM_ELEMENTS_H
#define FSM_ELEMENTS_H

#include "tree.hpp"

#include <vector>
#include <string>
#include <variant>
#include <optional>

#include "fmt/format.h"

namespace parser
{
    struct FSMElement
    {
        FSMElement() = default;
        FSMElement(
            std::string_view id)
            : m_id{id} {}

        auto operator<=>(const FSMElement &) const = default;

        std::string m_id;
    };

    struct FSMState : public FSMElement
    {
        FSMState() = default;
        
        FSMState(
            std::string_view id,
            std::string_view state_name,
            const std::vector<std::string> &outputs,
            const bool is_default_state
        )
            : FSMElement{id},
              m_state_name{state_name},
              m_outputs{outputs},
              m_is_default_state{is_default_state} {}
        
        FSMState(
            std::string_view id,
            const std::vector<std::string> &outputs,
            const bool is_default_state
        )
            : FSMElement{id},
              m_state_name{std::nullopt},
              m_outputs{outputs},
              m_is_default_state{is_default_state} {}
        
        FSMState(
            std::string_view id,
            std::string_view state_name,
            const bool is_default_state
        )
            : FSMElement{id},
              m_state_name{state_name},
              m_outputs{std::nullopt},
              m_is_default_state{is_default_state} {}

        FSMState(
            std::string_view id,
            const bool is_default_state
        )
            : FSMElement{id},
              m_state_name{std::nullopt},
              m_outputs{std::nullopt},
              m_is_default_state{is_default_state} {}

        auto operator<=>(const FSMState &) const = default;

        std::optional<std::string> m_state_name;
        std::optional<std::vector<std::string>> m_outputs;
        bool m_is_default_state;
    };

    struct FSMPredicate : public FSMElement
    {
        FSMPredicate() = default;
        FSMPredicate(
            std::string_view id,
            std::string_view variable,
            std::string_view comparator,
            std::string_view comparison_value
        )
            : FSMElement{id},
              m_variable{variable}, 
              m_comparator{comparator}, 
              m_comparison_value{comparison_value} {}
        FSMPredicate(
            std::string_view id,
            std::string_view variable
        )
            : FSMElement{id},
              m_variable{variable}, 
              m_comparator{std::nullopt}, 
              m_comparison_value{std::nullopt} {}

        auto operator<=>(const FSMPredicate &) const = default;

        auto to_string() -> std::string
        {
            if (m_comparator.has_value() && m_comparison_value.has_value())
            {
                return m_variable+m_comparator.value()+m_comparison_value.value();
            }
            return m_variable;
        }

        std::string m_variable;
        std::optional<std::string> m_comparator;
        std::optional<std::string> m_comparison_value;
    };

    struct FSMArrow : public FSMElement
    {
        FSMArrow() = default;
        FSMArrow(
            std::string_view id,
            std::string_view source,
            std::string_view target
        )
            : FSMElement{id},
              m_source{source},
              m_target{target},
              m_value{std::nullopt} {}

        FSMArrow(
            std::string_view id,
            std::string_view source,
            std::string_view target,
            const bool value
        )
            : FSMElement{id},
              m_source{source},
              m_target{target},
              m_value{value} {}

        auto operator<=>(const FSMArrow &) const = default;

        std::string m_source;
        std::string m_target;
        std::optional<bool> m_value;
    };

    struct FSMTransition : public FSMElement
    {
        FSMTransition() = default;
        // either its a state and has no predicate, only an idenitifying ID
        FSMTransition(
            std::string_view id
        )
            : FSMElement{id},
              m_predicate{std::nullopt} {}
              
        // or its a decision block in which case it its just a wrapper for the predicate really
        FSMTransition(
            const parser::FSMPredicate& predicate
        )
            : FSMElement{predicate.m_id},
              m_predicate{predicate} {}

        auto operator<=>(const FSMTransition &) const = default;

        std::optional<parser::FSMPredicate> m_predicate;
    };
}

using States_t = std::vector<parser::FSMState>;
using Arrows_t = std::vector<parser::FSMArrow>;
using Predicates_t = std::vector<parser::FSMPredicate>;
using Connections_t = std::vector<std::vector<std::optional<bool>>>;

using TokenTuple = std::tuple<States_t, Predicates_t, Arrows_t>;
using TransitionTree = utility::binary_tree<parser::FSMTransition>;
using TransitionNode = utility::Node<parser::FSMTransition>;
using StateTransitionMap = std::vector<std::pair<parser::FSMState, TransitionTree>>;

#endif