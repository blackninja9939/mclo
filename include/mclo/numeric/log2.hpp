#pragma once

#include "mclo/debug/assert.hpp"

#include <bit>
#include <concepts>

namespace mclo
{
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T log2_floor( const T value ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( value != 0, "log2(0) is undefined" );
		return static_cast<T>( std::bit_width( value ) - 1 );
	}

	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T log2_ceil( const T value ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( value != 0, "log2(0) is undefined" );
		return static_cast<T>( std::bit_width( value - 1 ) );
	}
}
