#ifndef OBSERVER_H
#define OBSERVER_H

#include <concepts>
#include <numeric>
#include <string>
#include <algorithm>
#include <memory>
#include <functional>
#include <utility>

namespace utility
{
    template <std::movable T>
    class Observed
    {
    public:
        // ctors
        Observed() 
            : m_data{nullptr}, m_updated{false} {};

        Observed(T &data) 
            : m_data{&data}, m_updated{true} {};

        Observed(T* data) 
            : m_data{data}, m_updated{true} {};

        // copy ctor
        Observed(const Observed& other) 
            : m_data{other.m_data},
              m_updated{other.m_updated} {}

        // move ctor
        Observed(Observed&& other)
            : m_data{other.m_data},
              m_updated{std::move(other.m_updated)} {}

        // copy assignment operator
        auto operator=(const T& other) -> T &
        {
            if (this != &other)
            {
                m_data    = other.m_data;
                m_updated = other.m_updated;
            }
            return *this;
        }

        // move assignment operator
        auto operator=(T &&other) -> T &
        {
            if (this != &other)
            {
                m_data    = std::exchange(other.m_data, {});
                m_updated = std::exchange(other.m_updated, true);
            }
            return *this;
        }

        // setting the data leaves it 'updated'
        //template <typename U> requires(std::same_as<T, U>)
        auto write(const T &value) -> void
        {
            *m_data = value;
            m_updated = true;
        }

        // if you want to update the value via a class method of T
        template<typename Ret, typename... Args, typename U>
        requires(std::same_as<T, U> && !std::is_integral_v<U>)
        auto call(Ret (U::*func)(Args...), Args... args) 
        {
            m_updated = true;
            return std::invoke(func, *m_data, args...);
        }

        // to test if it has since been modified (i.e. written without reading)
        auto modified() -> bool
        {
            return m_updated;
        }

        // reading the value means it has no longer been updated
        auto value() -> T &
        {
            m_updated = false;
            return *m_data;
        }

    private:
        T* m_data;
        bool m_updated;
    };
    
    auto any_of_modified(Observed<auto>&... observers)
    {
        return (observers.modified() || ...);
    }

}

#endif