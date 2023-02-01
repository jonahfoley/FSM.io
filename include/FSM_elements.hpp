#ifndef FSM_ELEMENTS_H
#define FSM_ELEMENTS_H

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
            const std::vector<std::string> &values)
            : FSMElement{id},
              m_values{values} {}

        std::vector<std::string> m_values;
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
              m_target{target} {}

        std::string m_source;
        std::string m_target;
    };

    using FSMToken = std::variant<FSMState, FSMDecision, FSMArrow>;

} // namespace parser



#endif