#ifndef FSM_ELEMENTS_H
#define FSM_ELEMENTS_H

#include <vector>
#include <string>
#include <variant>
#include <optional>

namespace parser
{
    struct FSMElement
    {
        FSMElement(
            std::string_view id)
            : m_id{id} {}

        auto operator<=>(const FSMElement&) const = default;

        std::string m_id;
    };

    struct FSMState : public FSMElement
    {
        FSMState(
            std::string_view id,
            const std::vector<std::string> &outputs)
            : FSMElement{id},
              m_outputs{outputs} {}

        auto operator<=>(const FSMState&) const = default;

        std::vector<std::string> m_outputs;
    };

    struct FSMPredicate : public FSMElement
    {
        FSMPredicate(
            std::string_view id,
            std::string_view predicate)
            : FSMElement{id},
              m_predicate{predicate} {}

        auto operator<=>(const FSMPredicate&) const = default;

        std::string m_predicate;
    };

    struct FSMArrow : public FSMElement
    {
        FSMArrow(
            std::string_view id,
            std::string_view source,
            std::string_view target)
            : FSMElement{id},
              m_source{source},
              m_target{target},
              m_value{std::nullopt} {}

        FSMArrow(
            std::string_view id,
            std::string_view source,
            std::string_view target,
            const bool value)
            : FSMElement{id},
              m_source{source},
              m_target{target},
              m_value{value} {}
            
        auto operator<=>(const FSMArrow&) const = default;

        std::string m_source;
        std::string m_target;
        std::optional<bool> m_value;
    };

    struct FSMDecision : public FSMElement
    {
        FSMDecision(
            std::string_view id,
            std::string_view from_path,
            std::string_view predicate,
            std::string_view true_path,
            std::string_view false_path)
            : FSMElement{id},
              m_from_path{from_path},  
              m_predicate{predicate},  
              m_true_path{true_path},  
              m_false_path{false_path} {}

        auto operator<=>(const FSMDecision&) const = default;

        std::string m_from_path;
        std::string m_predicate;
        std::string m_true_path;
        std::string m_false_path;
    };

    struct FSMTransition : public FSMElement
    {
        FSMTransition(
            std::string_view id)
            : FSMElement{id},
              m_predicate{std::nullopt} {}

        FSMTransition(
            std::string_view id,
            std::string_view predicate)
            : FSMElement{id},
              m_predicate{predicate} {}

        auto operator<=>(const FSMTransition&) const = default;

        std::optional<std::string> m_predicate;
    };

    using FSMToken = std::variant<FSMState, FSMArrow, FSMPredicate>;

} // namespace parser

#endif