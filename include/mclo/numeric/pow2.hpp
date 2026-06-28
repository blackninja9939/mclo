#pragma once

#include "mclo/debug/assert.hpp"

#include <bit>
#include <concepts>

namespace mclo
{
	/// @brief Computes two raised to the given exponent.
	/// @tparam T The unsigned integer type of the result.
	/// @param exponent The exponent to raise two to.
	/// @return The value @c 2^exponent.
	/// @pre @p exponent must be less than the bit width of @p T.
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T pow2( const T exponent ) noexcept
	{
		return T( 1 ) << exponent;
	}

	/// @brief Tests whether a value is a power of two.
	/// @tparam T The unsigned integer type of the value.
	/// @param value The value to test.
	/// @return @c true if @p value is a power of two, @c false otherwise. Zero is not a power of two.
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr bool is_pow2( const T value ) noexcept
	{
		return std::has_single_bit( value );
	}

	/// @brief Divides a value by @c 2^exponent.
	/// @tparam T The unsigned integer type of the value.
	/// @param value The value to divide.
	/// @param exponent The power-of-two exponent to divide by.
	/// @return @p value divided by @c 2^exponent.
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T divide_pow2( const T value, const T exponent ) noexcept
	{
		return value >> exponent;
	}

	/// @brief Computes a value modulo a power-of-two divisor.
	/// @tparam T The unsigned integer type of the value.
	/// @param value The value to take the modulo of.
	/// @param mod The divisor. Must be a power of two.
	/// @return The remainder of @p value divided by @p mod.
	/// @pre @p mod must be a power of two.
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T modulo_pow2( const T value, const T mod ) noexcept
	{
		MCLO_DEBUG_ASSERT( is_pow2( mod ), "Mod must be a power of 2" );
		return value & ( mod - 1 );
	}
}
