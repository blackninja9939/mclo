#pragma once

#include "mclo/numeric/pow2.hpp"

namespace mclo
{
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T align_up( const T value, const T align ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( is_pow2( align ), "Alignment must be a power of 2" );
		return ( value + align - 1 ) & ~( align - 1 );
	}
}
