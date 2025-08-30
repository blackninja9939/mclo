#pragma once

#include "mclo/debug/assert.hpp"

#include <concepts>
#include <limits>

namespace mclo
{
	template <std::integral T>
	[[nodiscard]] constexpr T ceil_divide( const T dividend, const T divisor ) noexcept
	{
		DEBUG_ASSERT( divisor != 0, "Division by zero" );
		if constexpr ( std::is_unsigned_v<T> )
		{
			// Branchless single div instruction
			return ( dividend + divisor - 1 ) / divisor;
		}
		else
		{
			// Branchless single idiv instruction as that gives remainder and quotient https://www.felixcloutier.com/x86/idiv
			// Adds 1 if there is a remainder and signs match via bit XOR to round up
			const T quotient = dividend / divisor;
			const T remainder = dividend % divisor;
			return quotient + ( ( remainder != 0 ) & ( ( dividend ^ divisor ) >= 0 ) );
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
