#pragma once

#include "standard_integer_type.hpp"

#include <utility>

namespace mclo
{
#ifdef __cpp_lib_integer_comparison_functions
	using std::cmp_equal;
	using std::cmp_less;
	using std::cmp_not_equal;
	using std::cmp_less_equal using std::cmp_greater;
	using std::cmp_greater_equal;
	using std::in_range;
#else
	template <typename T1, typename T2>
	[[nodiscard]] constexpr bool cmp_equal( const T1 left, const T2 right ) noexcept
	{
		static_assert( is_standard_integer<T1> && is_standard_integer<T2>,
					   "The integer comparison functions only accept standard and extended integer types." );
		if constexpr ( std::is_signed_v<T1> == std::is_signed_v<T2> )
		{
			return left == right;
		}
		else if constexpr ( std::is_signed_v<T2> )
		{
			return left == static_cast<std::make_unsigned_t<T2>>( right ) && right >= 0;
		}
		else
		{
			return static_cast<std::make_unsigned_t<T1>>( left ) == right && left >= 0;
		}
	}

	template <typename T1, typename T2>
	[[nodiscard]] constexpr bool cmp_not_equal( const T1 left, const T2 right ) noexcept
	{
		return !mclo::cmp_equal( left, right );
	}

	template <typename T1, typename T2>
	[[nodiscard]] constexpr bool cmp_less( const T1 left, const T2 right ) noexcept
	{
		static_assert( is_standard_integer<T1> && is_standard_integer<T2>,
					   "The integer comparison functions only accept standard and extended integer types." );
		if constexpr ( std::is_signed_v<T1> == std::is_signed_v<T2> )
		{
			return left < right;
		}
		else if constexpr ( std::is_signed_v<T2> )
		{
			return right > 0 && left < static_cast<std::make_unsigned_t<T2>>( right );
		}
		else
		{
			return left < 0 || static_cast<std::make_unsigned_t<T1>>( left ) < right;
		}
	}

	template <typename T1, typename T2>
	[[nodiscard]] constexpr bool cmp_greater( const T1 left, const T2 right ) noexcept
	{
		return mclo::cmp_less( right, left );
	}

	template <typename T1, typename T2>
	[[nodiscard]] constexpr bool cmp_less_equal( const T1 left, const T2 right ) noexcept
	{
		return !mclo::cmp_less( right, left );
	}

	template <typename T1, typename T2>
	[[nodiscard]] constexpr bool cmp_greater_equal( const T1 left, const T2 right ) noexcept
	{
		return !mclo::cmp_less( left, right );
	}

	namespace detail
	{
		template <typename T>
		[[nodiscard]] constexpr T min_limit() noexcept
		{                                            // same as (numeric_limits<T>::min)(), less throughput cost
			static_assert( is_standard_integer<T> ); // doesn't attempt to handle all types
			if constexpr ( std::is_signed_v<T> )
			{
				constexpr auto unsigned_max = static_cast<std::make_unsigned_t<T>>( -1 );
				return static_cast<T>( ( unsigned_max >> 1 ) + 1 ); // well-defined, N4950 [conv.integral]/3
			}
			else
			{
				return 0;
			}
		}

		template <typename T>
		[[nodiscard]] constexpr T max_limit() noexcept
		{                                            // same as (numeric_limits<T>::max)(), less throughput cost
			static_assert( is_standard_integer<T> ); // doesn't attempt to handle all types
			if constexpr ( std::is_signed_v<T> )
			{
				constexpr auto unsigned_max = static_cast<std::make_unsigned_t<T>>( -1 );
				return static_cast<T>( unsigned_max >> 1 );
			}
			else
			{
				return static_cast<T>( -1 );
			}
		}
	}

	template <typename Ret, typename T>
	[[nodiscard]] constexpr bool in_range( const T _Value ) noexcept
	{
		static_assert( is_standard_integer<Ret> && is_standard_integer<T>,
					   "The integer comparison functions only accept standard and extended integer types." );

		constexpr auto T_min = detail::min_limit<T>();
		constexpr auto Ret_min = detail::min_limit<Ret>();

		if constexpr ( mclo::cmp_less( T_min, Ret_min ) )
		{
			if ( _Value < T{ Ret_min } )
			{
				return false;
			}
		}

		constexpr auto T_max = detail::max_limit<T>();
		constexpr auto Ret_max = detail::max_limit<Ret>();

		if constexpr ( mclo::cmp_greater( T_max, Ret_max ) )
		{
			if ( _Value > T{ Ret_max } )
			{
				return false;
			}
		}

		return true;
	}
#endif
}
