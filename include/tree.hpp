#ifndef TREE_H
#define TREE_H

#include <memory>

namespace utility
{
    template <typename T>
    struct Node
    {
        Node() : m_value{}, m_left{nullptr}, m_right{nullptr} {}
        Node(const T &value) : m_value{value}, m_left{nullptr}, m_right{nullptr} {}

        T m_value;
        std::unique_ptr<Node<T>> m_left;
        std::unique_ptr<Node<T>> m_right;
    };

    template <typename T>
    class binary_tree
    {
    public:
        binary_tree()
            : m_root{nullptr} {};

        binary_tree(const T &value)
            : m_root{std::make_unique<Node<T>>(value)} {};

        binary_tree(std::unique_ptr<Node<T>> &&value)
            : m_root{std::move(value)} {};

        auto empty() -> bool { return m_root == nullptr; }

        auto insert(std::unique_ptr<Node<T>> &node, const T &value) -> void
        {
            node = std::make_unique<Node<T>>(value);
        }

        std::unique_ptr<Node<T>> m_root;
    };

}

#endif