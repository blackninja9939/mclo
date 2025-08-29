#pragma once

#include <concepts>
#include <limits>

namespace mclo
{
	template <std::integral T>
	[[nodiscard]] constexpr T ceil_divide( const T dividend, const T divisor ) noexcept
	{
		if constexpr ( std::is_unsigned_v<T> )
		{
			return ( dividend + divisor - 1 ) / divisor;
		}
		else
		{
			if ( dividend > 0 == divisor > 0 )
			{
				return ( dividend + divisor - 1 ) / divisor;
			}
			return dividend / divisor;
		}
	}

	template <std::integral T>
	[[nodiscard]] constexpr T round_down_to_multiple_of( const T value, const T multiple_of ) noexcept
	{
		return value - ( value % multiple_of );
	}

	template <std::integral T, std::integral U>
	[[nodiscard]] constexpr bool is_safe_addition( const T lhs, const U rhs ) noexcept
	{
		constexpr T max = std::numeric_limits<T>::max();
		constexpr T min = std::numeric_limits<T>::min();
		if ( rhs > 0 && lhs > max - rhs )
		{
			return false;
		}
		if ( rhs < 0 && lhs < min - rhs )
		{
			return false;
		}
		return true;
	}
}
