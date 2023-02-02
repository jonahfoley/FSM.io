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

        std::string m_id;
    };

    struct FSMState : public FSMElement
    {
        FSMState(
            std::string_view id,
            const std::vector<std::string_view> &outputs)
            : FSMElement{id},
              m_outputs{outputs} {}

        std::vector<std::string_view> m_outputs;
    };

    struct FSMDecision : public FSMElement
    {
        FSMDecision(
            std::string_view id,
            std::string_view predicate)
            : FSMElement{id},
              m_predicate{predicate} {}

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

        std::string m_source;
        std::string m_target;
        std::optional<bool> m_value;
    };

    struct FSMTransition
    {
        FSMTransition(
            FSMDecision decision,
            FSMArrow true_path,
            FSMArrow false_path)
            : m_decision{decision},
              m_true_path{true_path},
              m_false_path{false_path} {}

        FSMDecision m_decision;
        FSMArrow m_true_path;
        FSMArrow m_false_path;
    };

    using FSMToken = std::variant<FSMState, FSMTransition>;

} // namespace parser

#endif