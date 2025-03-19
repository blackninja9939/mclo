#pragma once

#include "mclo/numeric/detail/overflowing_intrinsics.hpp"

namespace mclo
{
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
