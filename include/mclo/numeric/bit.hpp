#pragma once

#include "mclo/concepts/always_false.hpp"
#include "mclo/platform/builtins.hpp"
#include "mclo/platform/compiler_detection.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cinttypes>
#include <concepts>
#include <limits>

#include <immintrin.h>

namespace mclo
{
#ifdef __cpp_lib_byteswap

	/// @brief Reverses the order of the bytes in an integer, swapping between big and little endian.
	/// @tparam T The integer type of the value.
	/// @param value The value whose bytes to reverse.
	/// @return @p value with its byte order reversed.
	using std::byteswap;
#else
	namespace detail
	{
		template <typename T>
		[[nodiscard]] constexpr T byteswap( const T value ) noexcept
		{
			auto bytes = std::bit_cast<std::array<std::byte, sizeof( T )>>( value );
			std::reverse( bytes.begin(), bytes.end() );
			return std::bit_cast<T>( bytes );
		}
	}

	/// @brief Reverses the order of the bytes in an integer, swapping between big and little endian.
	/// @tparam T The integer type of the value.
	/// @param value The value whose bytes to reverse.
	/// @return @p value with its byte order reversed.
	template <std::integral T>
	[[nodiscard]] constexpr T byteswap( const T value ) noexcept
	{
		if constexpr ( sizeof( T ) == 1 )
		{
			return value;
		}
		else
		{
#ifdef MCLO_COMPILER_MSVC // On MSVC at run time use their intrinsics
			if ( std::is_constant_evaluated() )
			{
				return detail::byteswap( value );
			}
			else
			{
				if constexpr ( sizeof( T ) == 2 )
				{
					return static_cast<T>( _byteswap_ushort( static_cast<unsigned short>( value ) ) );
				}
				else if constexpr ( sizeof( T ) == 4 )
				{
					return static_cast<T>( _byteswap_ulong( static_cast<unsigned long>( value ) ) );
				}
				else if constexpr ( sizeof( T ) == 8 )
				{
					return static_cast<T>( _byteswap_uint64( static_cast<unsigned long long>( value ) ) );
				}
				else
				{
					static_assert( mclo::always_false<T>, "Invalid integer size" );
				}
			}
#else                     // Else always use naive implementation
			return detail::byteswap( value );
#endif
		}
	}
#endif

	/// @brief Reverses the order of the bits in an integer.
	/// @tparam T The unsigned integer type of the value.
	/// @param value The value whose bits to reverse.
	/// @return @p value with the order of all its bits reversed.
	template <std::unsigned_integral T>
	constexpr T bit_reverse( T value ) noexcept
	{
#if defined( MCLO_COMPILER_GCC_COMPATIBLE ) && MCLO_HAS_BUILTIN( __builtin_bitreverse8 )
		if constexpr ( sizeof( T ) == 1 )
		{
			return __builtin_bitreverse8( value );
		}
		else if constexpr ( sizeof( T ) == 2 )
		{
			return __builtin_bitreverse16( value );
		}
		else if constexpr ( sizeof( T ) == 4 )
		{
			return __builtin_bitreverse32( value );
		}
		else if constexpr ( sizeof( T ) == 8 )
		{
			return __builtin_bitreverse64( value );
		}
		else
		{
			static_assert( mclo::always_false<T>, "Invalid integer size" );
		}
#else
		using promoted_t = std::conditional_t<sizeof( T ) <= 4, std::uint32_t, std::uint64_t>;
		promoted_t x = byteswap( value );

		// Reverse within each byte
		x = ( ( x >> 4 ) & 0x0F0F0F0F0F0F0F0FULL ) | ( ( x & 0x0F0F0F0F0F0F0F0FULL ) << 4 );
		x = ( ( x >> 2 ) & 0x3333333333333333ULL ) | ( ( x & 0x3333333333333333ULL ) << 2 );
		x = ( ( x >> 1 ) & 0x5555555555555555ULL ) | ( ( x & 0x5555555555555555ULL ) << 1 );

		return static_cast<T>( x );
#endif
	}

	/// @brief Tiles the lowest @p length bits of a value to fill the whole integer width.
	/// @details Each bit @c i of the result is taken from bit @c i % length of @p value, repeating the
	/// low @p length bit pattern across the full width of @p T.
	/// @tparam T The unsigned integer type of the value.
	/// @param value The value whose lowest @p length bits form the pattern to repeat.
	/// @param length The number of low bits that make up the repeating pattern.
	/// @return The repeating bit pattern expanded to fill @p T.
	template <std::unsigned_integral T>
	constexpr T bit_repeat( const T value, const int length ) noexcept
	{
		T result = 0;
		for ( int i = 0; i < std::numeric_limits<T>::digits; ++i )
		{
			result |= ( ( value >> ( i % length ) ) & 1 ) << i;
		}
		return result;
	}

	namespace detail
	{
		[[nodiscard]] bool has_bmi2() noexcept;

		template <std::unsigned_integral T>
		constexpr T bit_compress( const T x, const T m ) noexcept
		{
			T result = 0;
			for ( int i = 0, j = 0; i < std::numeric_limits<T>::digits; ++i )
			{
				const int mask_bit = ( m >> i ) & 1;
				result |= ( mask_bit & ( x >> i ) ) << j;
				j += static_cast<int>( mask_bit );
			}
			return result;
		}

		template <std::unsigned_integral T>
		constexpr T bit_expand( const T x, const T m ) noexcept
		{
			T result = 0;
			for ( int i = 0, j = 0; i < std::numeric_limits<T>::digits; ++i )
			{
				const int mask_bit = ( m >> i ) & 1;
				result |= ( mask_bit & ( x >> j ) ) << i;
				j += static_cast<int>( mask_bit );
			}
			return result;
		}
	}

	/// @brief Gathers the bits of a value selected by a mask into the contiguous low-order bits of the result.
	/// @details Equivalent to the x86 @c PEXT (parallel bit extract) operation, using the hardware instruction
	/// when BMI2 is available and falling back to a portable implementation otherwise.
	/// @tparam T The unsigned integer type of the operands.
	/// @param x The value to extract bits from.
	/// @param m The mask selecting which bits of @p x to gather.
	/// @return The bits of @p x where @p m is set, packed into the low-order bits in order.
	template <std::unsigned_integral T>
	constexpr T bit_compress( const T x, const T m ) noexcept
	{
		if ( std::is_constant_evaluated() )
		{
			return detail::bit_compress( x, m );
		}
		else if ( detail::has_bmi2() )
		{
			if constexpr ( sizeof( T ) <= 4 )
			{
				return static_cast<T>( _pext_u32( x, m ) );
			}
			else if constexpr ( sizeof( T ) == 8 )
			{
				return _pext_u64( x, m );
			}
			else
			{
				static_assert( mclo::always_false<T>, "Invalid integer size" );
			}
		}
		return detail::bit_compress( x, m );
	}

	/// @brief Scatters the contiguous low-order bits of a value into the positions selected by a mask.
	/// @details Equivalent to the x86 @c PDEP (parallel bit deposit) operation, using the hardware instruction
	/// when BMI2 is available and falling back to a portable implementation otherwise. It is the inverse of
	/// @ref bit_compress for the same mask.
	/// @tparam T The unsigned integer type of the operands.
	/// @param x The value whose low-order bits to deposit.
	/// @param m The mask selecting which result bits to fill.
	/// @return A value with the low-order bits of @p x placed at the positions where @p m is set.
	template <std::unsigned_integral T>
	constexpr T bit_expand( const T x, const T m ) noexcept
	{
		if ( std::is_constant_evaluated() )
		{
			return detail::bit_expand( x, m );
		}
		else if ( detail::has_bmi2() )
		{
			if constexpr ( sizeof( T ) <= 4 )
			{
				return static_cast<T>( _pdep_u32( x, m ) );
			}
			else if constexpr ( sizeof( T ) == 8 )
			{
				return _pdep_u64( x, m );
			}
			else
			{
				static_assert( mclo::always_false<T>, "Invalid integer size" );
			}
		}
		return detail::bit_expand( x, m );
	}
}
