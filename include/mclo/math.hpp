#pragma once

#include <bit>
#include <cassert>
#include <concepts>

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
	[[nodiscard]] constexpr T modulo_pow2( const T value, const T mod ) noexcept
	{
		assert( is_pow2( mod ) );
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
		constexpr std::size_t size( const T ( & )[ N ] ) noexcept
		{
			return N;
		}
	}

	[[nodiscard]] constexpr std::size_t pow10( const unsigned char exponent ) noexcept
	{
		assert( exponent < detail::size( detail::pow10s ) );
		return detail::pow10s[ exponent ];
	}
}
