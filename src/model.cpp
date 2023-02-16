#include "../include/model.hpp"

#include <fmt/format.h>

namespace model 
{
    
    // namespaces aliases
    namespace views  = std::views;
    namespace ranges = std::ranges;

    // type aliases
    using TransitionTree = utility::binary_tree<parser::FSMTransition>;
    using TransitionNode = utility::Node<parser::FSMTransition>;
    using TransitionMatrix  = std::vector<std::vector<std::optional<bool>>>;

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
        ) -> std::optional<std::string>
        {
            auto predicate = predicates | std::views::filter([&id](auto& predicate){
                return predicate.m_id == id;
            }); 
            if (predicate) 
            {
                return predicate.front().m_predicate;
                
            }
            return std::nullopt;
        };
    }

    auto build_transition_matrix(
        const std::vector<parser::FSMState>& states,
        const std::vector<parser::FSMPredicate>& predicates,
        const std::vector<parser::FSMArrow>& arrows
    ) -> TransitionMatrix
    {

        // generate the blank matrix
        unsigned dimension = states.size() + predicates.size();

        TransitionMatrix matrix;
        for (unsigned i = 0; i < dimension; i++)
        {
            std::vector<std::optional<bool>> row{};
            for (unsigned j = 0; j < dimension; j++)
            {
                row.push_back(std::nullopt);
            }
            matrix.push_back(row);
        }

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
                matrix[row][col] = arrow.m_value.value();
            }
            // otherwise its a state->state transition, which is always true
            else 
            {
                matrix[row][col] = true;
            }
        }   

        return matrix;
    }

    static 
    auto process_node(
        // about the states
        const unsigned dimensions,
        const std::vector<parser::FSMState>& states,
        const std::vector<parser::FSMPredicate>& predicates,
        const TransitionMatrix transition_matrix,
        // where we are
        const unsigned row, 
        const unsigned col,
        const std::unique_ptr<TransitionNode>& root
    ) -> void
    {        
        auto add_node = [&](std::unique_ptr<TransitionNode>& node)
        {
            std::string id = helpers::id_from_position(states, predicates, row);
            std::optional<std::string> pred = helpers::pred_from_id(predicates, id);
            
            // we are going to a decision block -> add the state and look at children
            if (pred) 
            {
                parser::FSMTransition transition(id, pred.value());
                node = std::make_unique<TransitionNode>(transition);
                // process each of the possible children nodes
                for (unsigned i = 0; i < dimensions; ++i)
                {
                    process_node(
                        dimensions, states, predicates, transition_matrix, 
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

        if (transition_matrix[row][col].has_value())
        {
            if (transition_matrix[row][col].value()) 
            {
                add_node(root->m_left);
            }
            else
            {
                add_node(root->m_right);
            }
        }
    }
    
    auto build_transition_tree(
        const std::vector<parser::FSMState>& states,
        const std::vector<parser::FSMPredicate>& predicates,
        const TransitionMatrix& transition_matrix
    ) -> std::vector<TransitionTree>
    {
        const unsigned dimensions = states.size() + predicates.size();

        // visit each position in the transition matrix corresponding to a state
        // for each, follow the path, until all branches lead to another state
        // proceed with the next position
        std::vector<TransitionTree> trees;
        for (unsigned row = 0; row < dimensions; ++row)
        {
            for (unsigned col = 0; col < states.size(); ++col)
            {           
                if (transition_matrix[row][col].has_value()) //we got a transition
                {                    
                    std::string id = helpers::id_from_position(states, predicates, col);
                    parser::FSMTransition root(id);

                    auto root_ptr = std::make_unique<TransitionNode>(root);
                    process_node(
                        dimensions, states, predicates, transition_matrix, 
                        row, col, root_ptr
                    );

                    trees.push_back(TransitionTree(std::move(root_ptr)));
                }
            }
        }
        return trees;
    }
}