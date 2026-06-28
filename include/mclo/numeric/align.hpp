#pragma once

#include "mclo/numeric/pow2.hpp"

namespace mclo
{
	/// @brief Rounds a value down to the nearest multiple of a power-of-two alignment.
	/// @tparam T The unsigned integer type of the value and alignment.
	/// @param value The value to align.
	/// @param align The alignment to round down to. Must be a power of two.
	/// @return The largest multiple of @p align that is less than or equal to @p value.
	/// @pre @p align must be a power of two.
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T align_down( const T value, const T align ) noexcept
	{
		MCLO_DEBUG_ASSERT( is_pow2( align ), "Alignment must be a power of 2" );
		return value & ~( align - 1 );
	}

	/// @brief Rounds a value up to the nearest multiple of a power-of-two alignment.
	/// @tparam T The unsigned integer type of the value and alignment.
	/// @param value The value to align.
	/// @param align The alignment to round up to. Must be a power of two.
	/// @return The smallest multiple of @p align that is greater than or equal to @p value.
	/// @pre @p align must be a power of two.
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T align_up( const T value, const T align ) noexcept
	{
		MCLO_DEBUG_ASSERT( is_pow2( align ), "Alignment must be a power of 2" );
		return align_down( static_cast<T>( value + align - 1 ), align );
	}

	/// @brief Tests whether a value is a multiple of a power-of-two alignment.
	/// @tparam T The unsigned integer type of the value and alignment.
	/// @param value The value to test.
	/// @param align The alignment to test against. Must be a power of two.
	/// @return @c true if @p value is a multiple of @p align, @c false otherwise.
	/// @pre @p align must be a power of two.
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr bool is_aligned( const T value, const T align ) noexcept
	{
		MCLO_DEBUG_ASSERT( is_pow2( align ), "Alignment must be a power of 2" );
		return modulo_pow2( value, align ) == 0;
	}
}
