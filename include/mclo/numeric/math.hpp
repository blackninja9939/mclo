#pragma once

#include "mclo/debug/assert.hpp"

#include <concepts>

namespace mclo
{
	/// @brief Divides two integers, rounding the quotient towards positive infinity.
	/// @tparam T The integer type of the operands.
	/// @param dividend The value to be divided.
	/// @param divisor The value to divide by.
	/// @return The quotient of @p dividend divided by @p divisor, rounded up.
	/// @pre @p divisor must not be zero.
	template <std::integral T>
	[[nodiscard]] constexpr T ceil_divide( const T dividend, const T divisor ) noexcept
	{
		MCLO_DEBUG_ASSERT( divisor != T( 0 ), "Division by zero" );

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

	/// @brief Rounds a value down to the nearest multiple of another value.
	/// @tparam T The integer type of the operands.
	/// @param value The value to round down.
	/// @param multiple_of The multiple to round down to.
	/// @return The largest multiple of @p multiple_of that is less than or equal to @p value.
	template <std::integral T>
	[[nodiscard]] constexpr T round_down_to_multiple_of( const T value, const T multiple_of ) noexcept
	{
		return value - ( value % multiple_of );
	}

	/// @brief Computes the largest integer not greater than a single-precision floating-point value.
	/// @param value The value to floor.
	/// @return @p value rounded towards negative infinity as a 32-bit integer.
	[[nodiscard]] constexpr std::int32_t floor_to_int( const float value ) noexcept
	{
		const std::int32_t trunc = static_cast<std::int32_t>( value );
		return trunc - ( trunc > value );
	}

	/// @brief Computes the smallest integer not less than a single-precision floating-point value.
	/// @param value The value to ceil.
	/// @return @p value rounded towards positive infinity as a 32-bit integer.
	[[nodiscard]] constexpr std::int32_t ceil_to_int( const float value ) noexcept
	{
		const std::int32_t trunc = static_cast<std::int32_t>( value );
		return trunc + ( trunc < value );
	}

	/// @brief Computes the largest integer not greater than a double-precision floating-point value.
	/// @param value The value to floor.
	/// @return @p value rounded towards negative infinity as a 64-bit integer.
	[[nodiscard]] constexpr std::int64_t floor_to_int( const double value ) noexcept
	{
		const std::int64_t trunc = static_cast<std::int64_t>( value );
		return trunc - ( trunc > value );
	}

	/// @brief Computes the smallest integer not less than a double-precision floating-point value.
	/// @param value The value to ceil.
	/// @return @p value rounded towards positive infinity as a 64-bit integer.
	[[nodiscard]] constexpr std::int64_t ceil_to_int( const double value ) noexcept
	{
		const std::int64_t trunc = static_cast<std::int64_t>( value );
		return trunc + ( trunc < value );
	}
}
