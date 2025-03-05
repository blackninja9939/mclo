#pragma once

#include "mclo/debug/assert.hpp"
#include "mclo/numeric/standard_integer_type.hpp"

#include <limits>
#include <utility>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace mclo
{
	namespace detail
	{
		template <standard_integral T>
		[[nodiscard]] constexpr bool add_overflow_manual( const T x, const T y, T& result ) noexcept
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
		[[nodiscard]] constexpr bool sub_overflow_manual( const T x, const T y, T& result ) noexcept
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
		[[nodiscard]] constexpr bool mul_overflow_manual( const T x, const T y, T& result ) noexcept
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
		[[nodiscard]] constexpr bool div_overflow_manual( const T x, const T y, T& result ) noexcept
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
		[[nodiscard]] constexpr bool add_overflow( const T x, const T y, T& result ) noexcept
		{
#ifdef _MSC_VER
			if ( std::is_constant_evaluated() )
			{
				return add_overflow_manual( x, y, result );
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
					return add_overflow_manual( x, y, result );
				}
			}
#else
			return __builtin_add_overflow( x, y, &result );
#endif
		}

		template <standard_integral T>
		[[nodiscard]] constexpr bool sub_overflow( const T x, const T y, T& result ) noexcept
		{
#ifdef _MSC_VER
			if ( std::is_constant_evaluated() )
			{
				return sub_overflow_manual( x, y, result );
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
					return sub_overflow_manual( x, y, result );
				}
			}
#else
			return __builtin_sub_overflow( x, y, &result );
#endif
		}

		template <standard_integral T>
		[[nodiscard]] constexpr bool mul_overflow( const T x, const T y, T& result ) noexcept
		{
#ifdef _MSC_VER
			if ( std::is_constant_evaluated() )
			{
				return mul_overflow_manual( x, y, result );
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
					return mul_overflow_manual( x, y, result );
				}
			}
#else
			return __builtin_mul_overflow( x, y, &result );
#endif
		}

		template <standard_integral T>
		[[nodiscard]] constexpr bool div_overflow( const T x, const T y, T& result ) noexcept
		{
#ifdef _MSC_VER
			// No intrinsics since division doesn't really overflow
			return div_overflow_manual( x, y, result );
#else
			return __builtin_div_overflow( x, y, &result );
#endif
		}
	}

	template <standard_integral T>
	[[nodiscard]] constexpr T add_sat( const T x, const T y ) noexcept
	{
		if ( T result; !detail::add_overflow( x, y, result ) )
		{
			return result;
		}
		if constexpr ( standard_unsigned_integral<T> )
		{
			return std::numeric_limits<T>::max();
		}
		else
		{
			if ( x > 0 )
			{
				return std::numeric_limits<T>::max();
			}
			else
			{
				return std::numeric_limits<T>::min();
			}
		}
	}

	template <standard_integral T>
	[[nodiscard]] constexpr T sub_sat( const T x, const T y ) noexcept
	{
		if ( T result; !detail::sub_overflow( x, y, result ) )
		{
			return result;
		}
		if constexpr ( standard_unsigned_integral<T> )
		{
			return std::numeric_limits<T>::min();
		}
		else
		{
			if ( x >= 0 )
			{
				return std::numeric_limits<T>::max();
			}
			else
			{
				return std::numeric_limits<T>::min();
			}
		}
	}

	template <standard_integral T>
	[[nodiscard]] constexpr T mul_sat( const T x, const T y ) noexcept
	{
		if ( T result; !detail::mul_overflow( x, y, result ) )
		{
			return result;
		}
		if constexpr ( standard_unsigned_integral<T> )
		{
			return std::numeric_limits<T>::max();
		}
		else
		{
			if ( ( x > 0 && y > 0 ) || ( x < 0 && y < 0 ) )
			{
				return std::numeric_limits<T>::max();
			}
			else
			{
				return std::numeric_limits<T>::min();
			}
		}
	}

	template <standard_integral T>
	[[nodiscard]] constexpr T div_sat( const T x, const T y ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( y != 0, "Division by 0 is undefined behaviour" );
		if ( T result; !detail::div_overflow( x, y, result ) )
		{
			return result;
		}
		if constexpr ( standard_unsigned_integral<T> )
		{
			return x / y;
		}
		else
		{
			if ( x == std::numeric_limits<T>::min() && y == T( -1 ) )
			{
				return std::numeric_limits<T>::max();
			}
			else
			{
				return x / y;
			}
		}
	}

	template <standard_integral T, standard_integral U>
	[[nodiscard]] constexpr T saturate_cast( const U x ) noexcept
	{
		if ( std::cmp_less( x, std::numeric_limits<T>::min() ) )
		{
			return std::numeric_limits<T>::min();
		}
		if ( std::cmp_greater( x, std::numeric_limits<T>::max() ) )
		{
			return std::numeric_limits<T>::max();
		}
		return static_cast<T>( x );
	}
}
