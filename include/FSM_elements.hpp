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
        FSMElement() = default;
        FSMElement(
            std::string_view id
        )
            : m_id{id} {}

        auto operator<=>(const FSMElement&) const = default;

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

        auto operator<=>(const FSMState&) const = default;

        std::optional<std::string> m_state_name;
        std::vector<std::string> m_outputs;
        bool m_is_default_state;
    };

    struct FSMPredicate : public FSMElement
    {
        FSMPredicate() = default;
        FSMPredicate(
            std::string_view id,
            std::string_view predicate
        )
            : FSMElement{id},
              m_predicate{predicate} {}

        auto operator<=>(const FSMPredicate&) const = default;

        std::string m_predicate;
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
            
        auto operator<=>(const FSMArrow&) const = default;

        std::string m_source;
        std::string m_target;
        std::optional<bool> m_value;
    };

    struct FSMDecision : public FSMElement
    {
        FSMDecision() = default;
        
        FSMDecision(
            std::string_view id,
            std::string_view from_path,
            std::string_view predicate,
            std::string_view true_path,
            std::string_view false_path
        )
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
        FSMTransition() = default;

        FSMTransition(
            std::string_view id
        )
            : FSMElement{id},
              m_predicate{std::nullopt} {}

        FSMTransition(
            std::string_view id,
            std::string_view predicate
        )
            : FSMElement{id},
              m_predicate{predicate} {}

        auto operator<=>(const FSMTransition&) const = default;

        std::optional<std::string> m_predicate;
    };
} 

#endif