#pragma once

#include <concepts>

namespace mclo
{
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T align_up( const T value, const T align ) noexcept
	{
		return ( value + align - 1 ) & ~( align - 1 );
	}
}
