#pragma once

#include "mclo/debug/assert.hpp"

#include <bit>
#include <concepts>

namespace mclo
{
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T pow2( const T exponent ) noexcept
	{
		return T( 1 ) << exponent;
	}

	template <std::unsigned_integral T>
	[[nodiscard]] constexpr bool is_pow2( const T value ) noexcept
	{
		return std::has_single_bit( value );
	}

	/// @brief Divide a value by 2^exponent
	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T divide_pow2( const T value, const T exponent ) noexcept
	{
		return value >> exponent;
	}

	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T modulo_pow2( const T value, const T mod ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( is_pow2( mod ), "Mod must be a power of 2" );
		return value & ( mod - 1 );
	}
}
