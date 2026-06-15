#pragma once

#include "mclo/debug/assert.hpp"

#include <concepts>

namespace mclo
{
	template <std::integral T>
	[[nodiscard]] constexpr T ceil_divide( const T dividend, const T divisor ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( divisor != T( 0 ), "Division by zero" );

		// Branchless single div/idiv instruction as that gives remainder and quotient
		// https://www.felixcloutier.com/x86/idiv
		const T quotient = dividend / divisor;
		const T remainder = dividend % divisor;

		// Add 1 if there is a remainder and if signs bits match
		// Use binary & for branchless and that very matters to MSVC
		if constexpr ( std::is_unsigned_v<T> )
		{
			return quotient + ( remainder != 0 );
		}
		else
		{
			return quotient + ( ( remainder != 0 ) & ( ( dividend ^ divisor ) >= 0 ) );
		}
	}

	template <std::integral T>
	[[nodiscard]] constexpr T round_down_to_multiple_of( const T value, const T multiple_of ) noexcept
	{
		return value - ( value % multiple_of );
	}

	[[nodiscard]] constexpr std::int32_t floor_to_int( const float value ) noexcept
	{
		const std::int32_t trunc = static_cast<std::int32_t>( value );
		return trunc - ( trunc > value );
	}

	[[nodiscard]] constexpr std::int32_t ceil_to_int( const float value ) noexcept
	{
		const std::int32_t trunc = static_cast<std::int32_t>( value );
		return trunc + ( trunc < value );
	}

	[[nodiscard]] constexpr std::int64_t floor_to_int( const double value ) noexcept
	{
		const std::int64_t trunc = static_cast<std::int64_t>( value );
		return trunc - ( trunc > value );
	}

	[[nodiscard]] constexpr std::int64_t ceil_to_int( const double value ) noexcept
	{
		const std::int64_t trunc = static_cast<std::int64_t>( value );
		return trunc + ( trunc < value );
	}
}
