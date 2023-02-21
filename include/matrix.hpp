#ifndef MATRIX_H
#define MATRIX_H

#include <array>

namespace utility
{
    template<typename T, unsigned N, unsigned M>
    class Matrix
    {
    public:
        Matrix() : m_data{{}} {}

        Matrix(const T& value) 
        {
            for (unsigned i = 0; i < N; i++) 
            {
                for (unsigned j = 0; j < M; j++)
                {
                    m_data[i][j] = value;
                }
            }
        }

        Matrix(T&& value)
        {
            for (unsigned i = 0; i < N; i++) 
            {
                for (unsigned j = 0; j < M; j++)
                {
                    m_data[i][j] = value;
                }
            }
        }

        Matrix (std::array<std::array<T, M>, N>&& data) 
            : m_data{std::move(data)} 
            {}

        Matrix (const Matrix<T, N, M>& other) 
            : m_data{other.m_data}
            {}

        Matrix (Matrix<T, N, M>&& other) 
            : m_data{std::move(other.m_data)} 
            {}

        T& operator() (unsigned row, unsigned col) 
        { 
            return m_data[row][col];
        }
        
        T operator() (unsigned row, unsigned col) const 
        { 
            return m_data[row][col];
        }

        auto num_elements() -> unsigned
        {
            return N * M;
        }

        auto num_rows() -> unsigned 
        {
            return N;
        }

        auto num_cols() -> unsigned
        {
            return M;
        }

    private:
        std::array<std::array<T, M>, N> m_data;
    };

    template<typename T, unsigned N> 
    using SquareMatrix = Matrix<T, N, N>;

}

#endif