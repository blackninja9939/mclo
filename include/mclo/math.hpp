#pragma once

#include <type_traits>

namespace mclo
{
	template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr T ceil_divide( const T dividend, const T divisor ) noexcept
	{
		if ( dividend > 0 == divisor > 0 )
		{
			return ( dividend + divisor - 1 ) / divisor;
		}
		else
		{
			return dividend / divisor;
		}
	}

	template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	constexpr T round_down_to_multiple_of(const T value, const T multiple_of) noexcept
	{
		return value - ( value % multiple_of );
	}
}
