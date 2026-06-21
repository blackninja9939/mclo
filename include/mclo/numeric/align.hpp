#pragma once

#include "mclo/numeric/pow2.hpp"

namespace mclo
{
	/// @brief Rounds a value up to the nearest multiple of a power-of-two alignment.
	/// @tparam T The unsigned integer type of the value and alignment.
	/// @param value The value to align.
	/// @param align The alignment to round up to. Must be a power of two.
	/// @return The smallest multiple of @p align that is greater than or equal to @p value.
	/// @pre @p align must be a power of two.
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T align_up( const T value, const T align ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( is_pow2( align ), "Alignment must be a power of 2" );
		return ( value + align - 1 ) & ~( align - 1 );
	}
}
