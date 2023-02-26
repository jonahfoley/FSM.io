#include "../include/transition_matrix.hpp"

#include <cassert>
#include <ranges>

#include <iostream>

namespace model 
{
    namespace views  = std::views;
    namespace ranges = std::ranges;

    TransitionMatrix::TransitionMatrix() 
        : m_connections{{}}, 
          m_rank{0} 
    {}

    TransitionMatrix::TransitionMatrix (
        const States_t& states, 
        const Arrows_t& arrows,
        const Predicates_t& predicates
    ) 
        : m_rank{static_cast<unsigned>(states.size()+predicates.size())},
          m_connections([&](){
              auto sz = states.size()+predicates.size();
              using matrix_t = decltype(m_connections);
              return matrix_t(sz, matrix_t::value_type(sz, std::nullopt));
          }())
    {
        populate_connection_matrix(states, arrows, predicates);
    }

    auto TransitionMatrix::populate_connection_matrix(
        const States_t& states, 
        const Arrows_t& arrows, 
        const Predicates_t& predicates
    ) -> void
    {
        auto find_pos = [&](std::string_view id)
        {
            auto get_id = [](auto& el){ return el.m_id; };

            unsigned pos;
            auto state_ids = states | views::transform(get_id);
            auto res = ranges::find(state_ids, id);
            if (res != state_ids.end())
            {
                pos = std::distance(state_ids.begin(), res);
            }
            else 
            {
                auto pred_ids = predicates | views::transform(get_id);
                auto res = ranges::find(pred_ids, id);
                pos = states.size() + std::distance(pred_ids.begin(), res);
            }
            return pos;
        };

        for(const auto& arrow : arrows)
        {
            // deduce the row & column
            unsigned col = find_pos(arrow.m_source);
            unsigned row = find_pos(arrow.m_target);

            // if its a decision node, the matrix value is the decision node value
            if (arrow.m_value)
            {
                this->set(row, col, arrow.m_value.value());
            }
            // otherwise its a state->state transition, which is always true
            else 
            {
                this->set(row, col, true);
            }
        }   
    } 

    auto TransitionMatrix::operator() (unsigned row, unsigned col) -> std::optional<bool>&
    { 
        assert(row < m_rank && col < m_rank); 
        return m_connections[row][col];
    }

    auto TransitionMatrix::operator() (unsigned row, unsigned col) const -> std::optional<bool>
    { 
        assert(row < m_rank && col < m_rank); 
        return m_connections[row][col];
    }

    auto TransitionMatrix::rank() -> unsigned
    {
        return m_rank;
    }

    auto TransitionMatrix::rank() const -> unsigned
    {
        return m_rank;
    }

    auto TransitionMatrix::set(const unsigned row, const unsigned col, const bool value) -> void
    {
        assert(row < m_rank && col < m_rank); // 0 .. rank-1
        m_connections[row][col] = value;
    }

    auto TransitionMatrix::print() -> void
    {
        for (const auto& row : m_connections)
        {
            for (const auto& elem : row)
            {
                if (elem.has_value()) fmt::print("{} ", elem.value());
                else fmt::print("null");
            }
            fmt::print("\n");
        }
    }

    auto TransitionMatrix::print() const -> void
    {
        for (const auto& row : m_connections)
        {
            for (const auto& elem : row)
            {
                if (elem.has_value()) fmt::print("{} ", elem.value());
                else fmt::print("null");
            }
            fmt::print("\n");
        }
    }

    namespace helpers
    {
        static auto id_from_position(
            const std::vector<parser::FSMState>& states,
            const std::vector<parser::FSMPredicate>& predicates,
            const unsigned pos
        ) -> std::string
        {
            if (pos < states.size()) 
            {
                return states[pos].m_id;
            }
            else 
            {
                auto offset_pos = pos - states.size();
                return predicates[offset_pos].m_id;
            }
        };

        static auto pred_from_id(
            const std::vector<parser::FSMPredicate>& predicates,
            const std::string_view id
        ) -> std::optional<parser::FSMPredicate>
        {
            auto predicate = predicates | std::views::filter([&id](auto& predicate){
                return predicate.m_id == id;
            }); 
            if (predicate) 
            {
                return predicate.front();
            }
            return std::nullopt;
        };

        static auto process_node(
            // about the states
            const std::vector<parser::FSMState>& states,
            const std::vector<parser::FSMPredicate>& predicates,
            const TransitionMatrix& transition_matrix,
            // where we are
            const unsigned row, 
            const unsigned col,
            const std::unique_ptr<TransitionNode>& root
        ) -> void
        {        
            auto add_node = [&](std::unique_ptr<TransitionNode>& node)
            {
                auto id = id_from_position(states, predicates, row);
                auto pred = pred_from_id(predicates, id);
                
                // we are going to a decision block -> add the state and look at children
                if (pred) 
                {
                    parser::FSMTransition transition(pred.value());
                    node = std::make_unique<TransitionNode>(transition);
                    // process each of the possible children nodes
                    for (unsigned i = 0; i < transition_matrix.rank(); ++i)
                    {
                        process_node(
                            states, predicates, transition_matrix, 
                            i, row, node
                        );
                    }
                }
                // we are going to a state -> add the state and terminate
                else 
                {
                    parser::FSMTransition transition(id);
                    node = std::make_unique<TransitionNode>(transition);
                }
            };

            if (transition_matrix(row,col).has_value())
            {
                if (transition_matrix(row,col).value()) 
                {
                    add_node(root->m_left);
                }
                else
                {
                    add_node(root->m_right);
                }
            }
        }
    }
    
    auto build_transition_tree_map(
        const States_t& states,
        const Predicates_t& predicates,
        const TransitionMatrix& transition_matrix
    ) -> StateTransitionMap
    {
        // visit each position in the transition matrix corresponding to a state
        // for each, follow the path, until all branches lead to another state
        // proceed with the next position
        StateTransitionMap state_tree_map;
        for (unsigned row = 0; row < transition_matrix.rank(); ++row)
        {
            for (unsigned col = 0; col < states.size(); ++col)
            {           
                if (transition_matrix(row,col).has_value()) //we got a transition
                {                    
                    std::string id = helpers::id_from_position(states, predicates, col);
                    parser::FSMTransition root(id);

                    auto root_ptr = std::make_unique<TransitionNode>(root);
                    helpers::process_node(
                        states, predicates, transition_matrix, 
                        row, col, root_ptr
                    );

                    state_tree_map.push_back({states[col], TransitionTree(std::move(root_ptr))});
                }
            }
        }
        return state_tree_map;
    }
}