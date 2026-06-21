#pragma once

#include "mclo/debug/assert.hpp"

#include <bit>
#include <concepts>

namespace mclo
{
	/// @brief Computes the floor of the base-2 logarithm of a value.
	/// @tparam T The unsigned integer type of the value.
	/// @param value The value to take the logarithm of.
	/// @return The largest integer @c n such that @c 2^n is less than or equal to @p value.
	/// @pre @p value must not be zero.
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T log2_floor( const T value ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( value != T( 0 ), "log2(0) is undefined" );
		return static_cast<T>( std::bit_width( value ) - 1 );
	}

	/// @brief Computes the ceiling of the base-2 logarithm of a value.
	/// @tparam T The unsigned integer type of the value.
	/// @param value The value to take the logarithm of.
	/// @return The smallest integer @c n such that @c 2^n is greater than or equal to @p value.
	/// @pre @p value must not be zero.
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T log2_ceil( const T value ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( value != T( 0 ), "log2(0) is undefined" );
		return static_cast<T>( std::bit_width( value - 1 ) );
	}
}
