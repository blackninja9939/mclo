#pragma once

#include "mclo/debug/assert.hpp"

#include <bit>
#include <concepts>
#include <limits>

namespace mclo
{
	template <std::integral T>
	[[nodiscard]] constexpr T ceil_divide( const T dividend, const T divisor ) noexcept
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

	template <std::integral T>
	[[nodiscard]] constexpr T round_down_to_multiple_of( const T value, const T multiple_of ) noexcept
	{
		return value - ( value % multiple_of );
	}

	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T log2_floor( const T value ) noexcept
	{
		return static_cast<T>( std::bit_width( value ) - 1 );
	}

	template <std::unsigned_integral T>
	[[nodiscard]] constexpr T log2_ceil( const T value ) noexcept
	{
		return log2_floor<T>( value - 1 ) + 1;
	}

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

	namespace detail
	{
		// More than this overflows std::size_t max
		inline constexpr std::size_t pow10s[] = {
			1ull,
			10ull,
			100ull,
			1000ull,
			10000ull,
			100000ull,
			1000000ull,
			10000000ull,
			100000000ull,
			1000000000ull,
			10000000000ull,
			100000000000ull,
			1000000000000ull,
			10000000000000ull,
			100000000000000ull,
			1000000000000000ull,
			10000000000000000ull,
			100000000000000000ull,
			1000000000000000000ull,
			10000000000000000000ull,
		};

		// To avoid including it via iterator and other large headers
		template <typename T, std::size_t N>
		[[nodiscard]] constexpr std::size_t size( const T ( & )[ N ] ) noexcept
		{
			return N;
		}
	}

	[[nodiscard]] constexpr std::size_t pow10( const unsigned char exponent ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( exponent < detail::size( detail::pow10s ), "Result would overflow std::size_t" );
		return detail::pow10s[ exponent ];
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
