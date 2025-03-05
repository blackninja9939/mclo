#pragma once

#include <bit>
#include <concepts>

namespace mclo
{
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T log2_floor( const T value ) noexcept
	{
		return static_cast<T>( std::bit_width( value ) - 1 );
	}

	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T log2_ceil( const T value ) noexcept
	{
		return log2_floor<T>( value - 1 ) + 1;
	}
}
