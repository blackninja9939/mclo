#pragma once

#include "constant_evaluated.hpp"
#include "platform.hpp"
#include "type_traits.hpp"

#if __cplusplus >= 202002L

#include <bit>

namespace mclo
{
	using std::bit_cast;
	using std::bit_ceil;
	using std::bit_floor;
	using std::bit_width;
	using std::countl_one;
	using std::countl_zero;
	using std::countr_one;
	using std::countr_zero;
	using std::has_single_bit;
	using std::popcount;
	using std::rotl;
	using std::rotr;
}

#else

#include <climits>
#include <cstddef>

namespace mclo
{
	template <typename To,
			  typename From,
			  std::enable_if_t<std::conjunction_v<std::bool_constant<sizeof( To ) == sizeof( From )>,
												  std::is_trivially_copyable<To>,
												  std::is_trivially_copyable<From>>,
							   int> = 0>
	[[nodiscard]] constexpr To bit_cast( const From& value ) noexcept
	{
#if MCLO_MSVC_OR_HAS_BUILTIN( __builtin_bit_cast )
		return __builtin_bit_cast( To, value );
#else
#error Platform does not support constexpr bit_cast intrinsic
#endif
	}

	namespace detail
	{
		template <typename T>
		constexpr int num_unsigned_digits = sizeof( T ) * CHAR_BIT;

		template <typename T>
		[[nodiscard]] constexpr int popcount( const T value ) noexcept
		{
			int count = 0;
			for ( ; value; ++count )
			{
				value &= value - 1;
			}
			return count;
		}

		template <typename T>
		[[nodiscard]] constexpr int countl_zero( T value ) noexcept
		{
			T y = 0;

			unsigned int n = num_unsigned_digits<T>;
			unsigned int c = num_unsigned_digits<T> / 2;
			do
			{
				y = static_cast<T>( value >> c );
				if ( y != 0 )
				{
					n -= c;
					value = y;
				}
				c >>= 1;
			}
			while ( c != 0 );

			return static_cast<int>( n ) - static_cast<int>( value );
		}

		template <typename T>
		[[nodiscard]] constexpr int countr_zero( const T value ) noexcept
		{
			return num_unsigned_digits<T> -
				   countl_zero( static_cast<T>( static_cast<T>( ~value ) & static_cast<T>( value - 1 ) ) );
		}

		template <typename T>
		[[nodiscard]] constexpr T rotr( const T value, const int shift ) noexcept;

		template <typename T>
		[[nodiscard]] constexpr T rotl( const T value, const int shift ) noexcept
		{
			constexpr int digits = num_unsigned_digits<T>;
			const int remainder = shift % digits;
			if ( remainder > 0 )
			{
				return static_cast<T>( static_cast<T>( value << remainder ) |
									   static_cast<T>( value >> ( digits - remainder ) ) );
			}
			else if ( remainder == 0 )
			{
				return value;
			}
			else
			{
				return rotr( value, -remainder );
			}
		}

		template <typename T>
		[[nodiscard]] constexpr T rotr( const T value, const int shift ) noexcept
		{
			constexpr int digits = num_unsigned_digits<T>;
			const int remainder = shift % digits;
			if ( remainder > 0 )
			{
				return static_cast<T>( static_cast<T>( value >> remainder ) |
									   static_cast<T>( value << ( digits - remainder ) ) );
			}
			else if ( remainder == 0 )
			{
				return value;
			}
			else
			{
				return rotl( value, -remainder );
			}
		}
	}

	// counting
	template <typename T>
	[[nodiscard]] constexpr int countl_zero( const T value ) noexcept
	{
#ifdef _MSC_VER // MSVC use intrinsics at runtime
		if ( mclo::is_constant_evaluated() )
		{
			return detail::countl_zero( value );
		}
		else
		{
			constexpr int digits = detail::num_unsigned_digits<T>;
			if constexpr ( digits <= 32 )
			{
#if __AVX__
				if constexpr ( digits <= 16 )
				{
					return static_cast<int>( __lzcnt16( value ) - ( 16 - digits ) );
				}
				else
				{
					return static_cast<int>( __lzcnt( value ) );
				}
#else
				unsigned long result;
				if ( _BitScanReverse( &result, value ) == 0 )
				{
					return digits;
				}
				return static_cast<int>( digits - 1 - result );
#endif
			}
			else
			{
#if __AVX__
				return static_cast<int>( __lzcnt64( value ) );
#else
				unsigned long result;
				if ( _BitScanReverse64( &result, value ) == 0 )
				{
					return digits;
				}
				return static_cast<int>( digits - 1 - result );
#endif
			}
		}
#else
		return detail::countl_zero( value );
#endif
	}

	template <typename T>
	[[nodiscard]] constexpr int countl_one( const T value ) noexcept
	{
		return countl_zero( static_cast<T>( ~value ) );
	}

	template <typename T>
	[[nodiscard]] constexpr int countr_zero( const T value ) noexcept
	{
#ifdef _MSC_VER // MSVC use intrinsics at runtime
		if ( mclo::is_constant_evaluated() )
		{
			return detail::countr_zero( value );
		}
		else
		{
			constexpr int digits = detail::num_unsigned_digits<T>;
			if constexpr ( digits <= 32 )
			{
				const auto widened_value = static_cast<unsigned int>( ~T( -1 ) | value );
#if __AVX__
				return static_cast<int>( _tzcnt_u32( widened_value ) );
#else
				unsigned long result;
				if ( _BitScanForward( &result, widened_value ) == 0 )
				{
					return digits;
				}
				return static_cast<int>( result );
#endif
			}
			else
			{
#if __AVX__
				return static_cast<int>( _tzcnt_u64( value ) );
#else
				unsigned long result;
				if ( _BitScanForward64( &result, value ) == 0 )
				{
					return digits;
				}
				return static_cast<int>( result );
#endif
			}
		}
#else
		return detail::countr_zero( value );
#endif
	}

	template <typename T>
	[[nodiscard]] constexpr int countr_one( const T value ) noexcept
	{
		return countr_zero( static_cast<T>( ~value ) );
	}

	template <typename T>
	[[nodiscard]] constexpr int popcount( const T value ) noexcept
	{
#ifdef _MSC_VER // MSVC use intrinsics at runtime
		if ( mclo::is_constant_evaluated() )
		{
			return detail::popcount( value );
		}
		else
		{
			constexpr int digits = detail::num_unsigned_digits<T>;
			if constexpr ( digits <= 16 )
			{
				return __popcnt16( value );
			}
			else if constexpr ( digits == 32 )
			{
				return __popcnt( value );
			}
			else
			{
				return __popcnt64( value );
			}
		}
#else
		return detail::popcount( value );
#endif
	}

	// integral powers of 2
	template <typename T, typename = std::enable_if_t<is_standard_unsigned_integer<T>>>
	[[nodiscard]] constexpr bool has_single_bit( const T value ) noexcept
	{
		return value && !( value & ( value - 1 ) );
	}

	template <typename T, typename = std::enable_if_t<is_standard_unsigned_integer<T>>>
	[[nodiscard]] constexpr T bit_ceil( const T value ) noexcept
	{
		if ( value <= 1u )
		{
			return T( 1 );
		}

		constexpr int digits = detail::num_unsigned_digits<T>;
		const int num = digits - mclo::countl_zero( static_cast<T>( value - 1 ) );

		// Possible integral promotion
		if constexpr ( sizeof( T ) < sizeof( unsigned int ) )
		{
			if ( mclo::is_constant_evaluated() )
			{
				if ( num == digits )
				{
					static_assert( mclo::always_false<T>,
								   "Standard forbids undefined behaviour in constant expression" );
				}
			}
		}

		return static_cast<T>( T( 1 ) << num );
	}

	template <typename T, typename = std::enable_if_t<is_standard_unsigned_integer<T>>>
	[[nodiscard]] constexpr int bit_width( const T value ) noexcept
	{
		return detail::num_unsigned_digits<T> - mclo::countl_zero( value );
	}

	template <typename T, typename = std::enable_if_t<is_standard_unsigned_integer<T>>>
	[[nodiscard]] constexpr T bit_floor( const T value ) noexcept
	{
		return value == 0 ? 0 : ( T( 1 ) << ( mclo::bit_width( value ) - 1 ) );
	}

	// rotating
	template <typename T>
	[[nodiscard]] constexpr T rotl( const T value, const int shift ) noexcept
	{
#ifdef _MSC_VER // MSVC uses intrinsics
		if ( mclo::is_constant_evaluated() )
		{
			return detail::rotl( value, shift );
		}
		else
		{
			constexpr int digits = detail::num_unsigned_digits<T>;
			if constexpr ( digits == 64 )
			{
				return _rotl64( value, shift )
			}
			else if constexpr ( digits == 32 )
			{
				return _rotl( value, shift );
			}
			else if constexpr ( digits == 16 )
			{
				return _rotl16( value, static_cast<unsigned char>( shift ) );
			}
			else if constexpr ( digits == 8 )
			{
				return _rotl8( value, static_cast<unsigned char>( shift ) );
			}
			else
			{
				static_assert( mclo::always_false<T>, "Unhandled number of digits" );
			}
		}
#else
		return detail::rotl( value, shift );
#endif
	}

	template <typename T>
	[[nodiscard]] constexpr T rotr( const T value, const int shift ) noexcept
	{
#ifdef _MSC_VER // MSVC uses intrinsics
		if ( mclo::is_constant_evaluated() )
		{
			return detail::rotr( value, shift );
		}
		else
		{
			constexpr int digits = detail::num_unsigned_digits<T>;
			if constexpr ( digits == 64 )
			{
				return _rotr64( value, shift )
			}
			else if constexpr ( digits == 32 )
			{
				return _rotr( value, shift );
			}
			else if constexpr ( digits == 16 )
			{
				return _rotr16( value, static_cast<unsigned char>( shift ) );
			}
			else if constexpr ( digits == 8 )
			{
				return _rotr8( value, static_cast<unsigned char>( shift ) );
			}
			else
			{
				static_assert( mclo::always_false<T>, "Unhandled number of digits" );
			}
		}
#else
		return detail::rotr( value, shift );
#endif
	}
}

#endif

#ifdef __cpp_lib_byteswap
namespace mclo
{
	using std::byteswap;
}
#else

#include "algorithm.hpp"

namespace mclo
{
	namespace detail
	{
		template <typename T>
		[[nodiscard]] constexpr T byteswap( const T value ) noexcept
		{
			auto bytes = mclo::bit_cast<std::array<std::byte, sizeof( T )>>( value );
			mclo::reverse( bytes.begin(), bytes.end() );
			return mclo::bit_cast<T>( bytes );
		}
	}

	template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
	[[nodiscard]] constexpr T byteswap( const T value ) noexcept
	{
		if constexpr ( sizeof( T ) == 1 )
		{
			return value;
		}
		else
		{
#ifdef _MSC_VER // On MSVC at run time use their intrinsics
			if ( mclo::is_constant_evaluated() )
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
#else           // Else always use naive implementation
			return detail::byteswap( value );
#endif
		}
	}
}
#endif
