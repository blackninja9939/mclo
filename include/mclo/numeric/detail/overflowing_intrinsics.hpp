#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/numeric/standard_integer_type.hpp"
#include "mclo/platform/compiler_detection.hpp"

#include <limits>
#include <utility>

#ifdef MCLO_COMPILER_MSVC
#include <intrin.h>
#endif

namespace mclo::detail
{
	template <standard_integral T>
	[[nodiscard]] constexpr bool overflowing_add_manual( const T x, const T y, T& result ) noexcept
	{
		if constexpr ( standard_unsigned_integral<T> )
		{
			result = x + y;
			return result < x;
		}
		else
		{
			using uint_type = std::make_unsigned_t<T>;
			result = static_cast<T>( static_cast<uint_type>( x ) + static_cast<uint_type>( y ) );
			return ( x > 0 && y > 0 && result <= 0 ) || ( x < 0 && y < 0 && result >= 0 );
		}
	}

	template <standard_integral T>
	[[nodiscard]] constexpr bool overflowing_sub_manual( const T x, const T y, T& result ) noexcept
	{
		if constexpr ( standard_unsigned_integral<T> )
		{
			result = x - y;
			return x < y;
		}
		else
		{
			using uint_type = std::make_unsigned_t<T>;
			result = static_cast<T>( static_cast<uint_type>( x ) - static_cast<uint_type>( y ) );
			return ( y > 0 && x < std::numeric_limits<T>::min() + y ) ||
				   ( y < 0 && x > std::numeric_limits<T>::max() + y );
		}
	}

	template <standard_integral T>
	[[nodiscard]] constexpr bool overflowing_mul_manual( const T x, const T y, T& result ) noexcept
	{
		if ( x == 0 || y == 0 )
		{
			result = 0;
			return false;
		}
		if constexpr ( standard_unsigned_integral<T> )
		{
			// Branch to avoid overflow in constant evaluation which is a compile error
			if ( y > std::numeric_limits<T>::max() / x )
			{
				return true;
			}
			result = x * y;
			return false;
		}
		else
		{
			using uint_type = std::make_unsigned_t<T>;
			const uint_type left_uint =
				static_cast<uint_type>( x < 0 ? ( 0 - static_cast<uint_type>( x ) ) : static_cast<uint_type>( x ) );
			const uint_type right_uint =
				static_cast<uint_type>( y < 0 ? ( 0 - static_cast<uint_type>( y ) ) : static_cast<uint_type>( y ) );
			const uint_type result_uint = static_cast<uint_type>( left_uint * right_uint );

			const bool negative = ( x < 0 ) != ( y < 0 );
			result = static_cast<T>( negative ? ( 0 - result_uint ) : result_uint );
			if ( left_uint == 0 || right_uint == 0 )
			{
				return false;
			}

			constexpr auto int_max = static_cast<uint_type>( std::numeric_limits<T>::max() );
			if ( negative )
			{
				return left_uint > ( int_max + uint_type{ 1 } ) / right_uint;
			}
			else
			{
				return left_uint > int_max / right_uint;
			}
		}
	}

	template <standard_integral T>
	[[nodiscard]] constexpr bool overflowing_div_manual( const T x, const T y, T& result ) noexcept
	{
		if constexpr ( standard_unsigned_integral<T> )
		{
			result = x / y;
			return false;
		}
		else
		{
			if ( x == std::numeric_limits<T>::min() && y == -1 )
			{
				return true;
			}
			result = x / y;
			return false;
		}
	}

	template <standard_integral T>
	[[nodiscard]] constexpr bool overflowing_add( const T x, const T y, T& result ) noexcept
	{
#ifdef MCLO_COMPILER_MSVC
		if ( std::is_constant_evaluated() )
		{
			return overflowing_add_manual( x, y, result );
		}
		else
		{
			if constexpr ( std::is_same_v<T, signed char> )
			{
				return _add_overflow_i8( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, signed short> )
			{
				return _add_overflow_i16( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, signed int> )
			{
				return _add_overflow_i32( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, signed __int64> )
			{
				return _add_overflow_i64( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, unsigned char> )
			{
				return _addcarry_u8( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, unsigned short> )
			{
				return _addcarry_u16( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, unsigned int> )
			{
				return _addcarry_u32( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, unsigned __int64> )
			{
				return _addcarry_u64( 0, x, y, &result );
			}
			else
			{
				return overflowing_add_manual( x, y, result );
			}
		}
#else
		return __builtin_add_overflow( x, y, &result );
#endif
	}

	template <standard_integral T>
	[[nodiscard]] constexpr bool overflowing_sub( const T x, const T y, T& result ) noexcept
	{
#ifdef MCLO_COMPILER_MSVC
		if ( std::is_constant_evaluated() )
		{
			return overflowing_sub_manual( x, y, result );
		}
		else
		{
			if constexpr ( std::is_same_v<T, signed char> )
			{
				return _sub_overflow_i8( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, signed short> )
			{
				return _sub_overflow_i16( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, signed int> )
			{
				return _sub_overflow_i32( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, signed __int64> )
			{
				return _sub_overflow_i64( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, unsigned char> )
			{
				return _subborrow_u8( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, unsigned short> )
			{
				return _subborrow_u16( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, unsigned int> )
			{
				return _subborrow_u32( 0, x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, unsigned __int64> )
			{
				return _subborrow_u64( 0, x, y, &result );
			}
			else
			{
				return overflowing_sub_manual( x, y, result );
			}
		}
#else
		return __builtin_sub_overflow( x, y, &result );
#endif
	}

	template <standard_integral T>
	[[nodiscard]] constexpr bool overflowing_mul( const T x, const T y, T& result ) noexcept
	{
#ifdef MCLO_COMPILER_MSVC
		if ( std::is_constant_evaluated() )
		{
			return overflowing_mul_manual( x, y, result );
		}
		else
		{
			if constexpr ( std::is_same_v<T, signed char> )
			{
				signed short value;
				const bool overflowed = _mul_full_overflow_i8( x, y, &value );
				result = static_cast<T>( value );
				return overflowed;
			}
			else if constexpr ( std::is_same_v<T, signed short> )
			{
				return _mul_overflow_i16( x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, signed int> )
			{
				return _mul_overflow_i32( x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, signed __int64> )
			{
				return _mul_overflow_i64( x, y, &result );
			}
			else if constexpr ( std::is_same_v<T, unsigned char> )
			{
				unsigned short value;
				const bool overflowed = _mul_full_overflow_u8( x, y, &value );
				result = static_cast<T>( value );
				return overflowed;
			}
			else if constexpr ( std::is_same_v<T, unsigned short> )
			{
				T high;
				return _mul_full_overflow_u16( x, y, &result, &high );
			}
			else if constexpr ( std::is_same_v<T, unsigned int> )
			{
				T high;
				return _mul_full_overflow_u32( x, y, &result, &high );
			}
			else if constexpr ( std::is_same_v<T, unsigned __int64> )
			{
				T high;
				return _mul_full_overflow_u64( x, y, &result, &high );
			}
			else
			{
				return overflowing_mul_manual( x, y, result );
			}
		}
#else
		return __builtin_mul_overflow( x, y, &result );
#endif
	}

	template <standard_integral T>
	[[nodiscard]] constexpr bool overflowing_div( const T x, const T y, T& result ) noexcept
	{
		MCLO_DEBUG_ASSERT( y != 0, "Division by 0 is undefined behaviour" );
		// No intrinsics since division doesn't really overflow
		return overflowing_div_manual( x, y, result );
	}
}
