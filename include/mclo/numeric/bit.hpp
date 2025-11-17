#pragma once

#include "mclo/concepts/always_false.hpp"
#include "mclo/preprocessor/platform.hpp"
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
