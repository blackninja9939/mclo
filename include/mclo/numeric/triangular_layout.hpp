#pragma once

#include <cmath>
#include <cstddef>
#include <utility>

namespace mclo
{
	/// @brief Returns the Nth triangular number, equal to the number of elements in the lower triangle of an N x N
	/// matrix including the diagonal.
	/// @param n The dimension of the matrix, or the index of the triangular number.
	/// @return n * (n + 1) / 2.
	[[nodiscard]] constexpr std::size_t triangular_size( const std::size_t n ) noexcept
	{
		return ( n * ( n + 1 ) ) / 2;
	}

	/// @brief Computes the flat index into a lower-triangular storage layout for a symmetric matrix.
	/// @details The arguments are order-independent, triangular_index(i, j) == triangular_index(j, i).
	/// @param row The row of the matrix element.
	/// @param column The column of the matrix element.
	/// @return The flat index corresponding to the element at (row, column).
	/// @see triangular_row_column
	[[nodiscard]] constexpr std::size_t triangular_index( std::size_t row, std::size_t column ) noexcept
	{
		if ( row < column )
		{
			std::swap( row, column );
		}
		return triangular_size( row ) + column;
	}

	/// @brief Recovers the row and column from a flat index into a lower-triangular storage layout.
	/// @details This is the inverse of triangular_index. For any valid row and column where row >= column,
	/// triangular_row_column(triangular_index(row, column)) == {row, column}.
	/// @param index The flat index to convert.
	/// @return A pair of {row, column}.
	/// @see triangular_index
	[[nodiscard]] inline std::pair<std::size_t, std::size_t> triangular_row_column( const std::size_t index ) noexcept
	{
		// Once implementations make cmath properly constexpr this can be constexpr not inline
		const std::size_t row = static_cast<std::size_t>( ( std::sqrt( 1.0 + 8.0 * index ) - 1.0 ) / 2.0 );
		return { row, index - triangular_size( row ) };
	}
}
