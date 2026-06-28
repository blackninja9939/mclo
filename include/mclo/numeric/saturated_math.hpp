#pragma once

#include "mclo/numeric/detail/overflowing_intrinsics.hpp"

namespace mclo
{
	/// @brief Adds two integers, clamping the result on overflow instead of wrapping.
	/// @tparam T The integer type of the operands.
	/// @param x The first operand.
	/// @param y The second operand.
	/// @return The sum of @p x and @p y, clamped to the range of @p T if the addition overflowed.
	template <standard_integral T>
	[[nodiscard]] constexpr T saturating_add( const T x, const T y ) noexcept
	{
		if ( T result; !detail::overflowing_add( x, y, result ) )
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

	/// @brief Subtracts two integers, clamping the result on overflow instead of wrapping.
	/// @tparam T The integer type of the operands.
	/// @param x The value to subtract from.
	/// @param y The value to subtract.
	/// @return The result of @p x minus @p y, clamped to the range of @p T if the subtraction overflowed.
	template <standard_integral T>
	[[nodiscard]] constexpr T saturating_sub( const T x, const T y ) noexcept
	{
		if ( T result; !detail::overflowing_sub( x, y, result ) )
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

	/// @brief Multiplies two integers, clamping the result on overflow instead of wrapping.
	/// @tparam T The integer type of the operands.
	/// @param x The first operand.
	/// @param y The second operand.
	/// @return The product of @p x and @p y, clamped to the range of @p T if the multiplication overflowed.
	template <standard_integral T>
	[[nodiscard]] constexpr T saturating_mul( const T x, const T y ) noexcept
	{
		if ( T result; !detail::overflowing_mul( x, y, result ) )
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

	/// @brief Divides two integers, clamping the result on overflow instead of wrapping.
	/// @tparam T The integer type of the operands.
	/// @param x The dividend.
	/// @param y The divisor.
	/// @return The result of @p x divided by @p y, clamped to the range of @p T if the division overflowed.
	/// @pre @p y must not be zero.
	template <standard_integral T>
	[[nodiscard]] constexpr T saturating_div( const T x, const T y ) noexcept
	{
		if ( T result; !detail::overflowing_div( x, y, result ) )
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

	/// @brief Casts an integer to a different integer type, clamping to the target range instead of wrapping.
	/// @tparam T The integer type to cast to.
	/// @tparam U The integer type of the input value.
	/// @param x The value to cast.
	/// @return @p x converted to @p T, clamped to the representable range of @p T.
	template <standard_integral T, standard_integral U>
	[[nodiscard]] constexpr T saturating_cast( const U x ) noexcept
	{
		/*
		 * Unwrapped version of effectively:
		 * if ( std::cmp_less( x, std::numeric_limits<T>::min() ) )
		 * {
		 * 	return std::numeric_limits<T>::min();
		 * }
		 * if ( std::cmp_greater( x, std::numeric_limits<T>::max() ) )
		 * {
		 * 	return std::numeric_limits<T>::max();
		 * }
		 *
		 * Unwrapping lets us eliminate the branches entirely at compile time
		 */
		constexpr int digits_result = std::numeric_limits<T>::digits;
		constexpr int digits_in = std::numeric_limits<U>::digits;
		constexpr T max_result = std::numeric_limits<T>::max();

		if constexpr ( std::is_signed_v<T> == std::is_signed_v<U> )
		{
			if constexpr ( digits_result < digits_in )
			{
				constexpr T min_result = std::numeric_limits<T>::lowest();

				if ( x < static_cast<U>( min_result ) )
				{
					return min_result;
				}
				else if ( x > static_cast<U>( max_result ) )
				{
					return max_result;
				}
			}
		}
		else if constexpr ( std::is_signed_v<U> )
		{
			if ( x < 0 )
			{
				return 0;
			}
			else if ( std::make_unsigned_t<U>( x ) > max_result )
			{
				return max_result;
			}
		}
		else
		{
			if ( x > std::make_unsigned_t<T>( max_result ) )
			{
				return max_result;
			}
		}
		return static_cast<T>( x );
	}
}
