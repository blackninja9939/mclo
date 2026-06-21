#pragma once

#include "mclo/numeric/detail/overflowing_intrinsics.hpp"

#include <optional>

namespace mclo
{
	/// @brief Adds two integers, checking for overflow.
	/// @tparam T The integer type of the operands.
	/// @param x The first operand.
	/// @param y The second operand.
	/// @return The sum of @p x and @p y, or @c std::nullopt if the addition overflowed.
	template <standard_integral T>
	[[nodiscard]] constexpr std::optional<T> checked_add( const T x, const T y ) noexcept
	{
		if ( T result; !detail::overflowing_add( x, y, result ) )
		{
			return result;
		}
		return std::nullopt;
	}

	/// @brief Subtracts two integers, checking for overflow.
	/// @tparam T The integer type of the operands.
	/// @param x The value to subtract from.
	/// @param y The value to subtract.
	/// @return The result of @p x minus @p y, or @c std::nullopt if the subtraction overflowed.
	template <standard_integral T>
	[[nodiscard]] constexpr std::optional<T> checked_sub( const T x, const T y ) noexcept
	{
		if ( T result; !detail::overflowing_sub( x, y, result ) )
		{
			return result;
		}
		return std::nullopt;
	}

	/// @brief Multiplies two integers, checking for overflow.
	/// @tparam T The integer type of the operands.
	/// @param x The first operand.
	/// @param y The second operand.
	/// @return The product of @p x and @p y, or @c std::nullopt if the multiplication overflowed.
	template <standard_integral T>
	[[nodiscard]] constexpr std::optional<T> checked_mul( const T x, const T y ) noexcept
	{
		if ( T result; !detail::overflowing_mul( x, y, result ) )
		{
			return result;
		}
		return std::nullopt;
	}

	/// @brief Divides two integers, checking for overflow.
	/// @tparam T The integer type of the operands.
	/// @param x The dividend.
	/// @param y The divisor.
	/// @return The result of @p x divided by @p y, or @c std::nullopt if the division overflowed.
	/// @pre @p y must not be zero.
	template <standard_integral T>
	[[nodiscard]] constexpr std::optional<T> checked_div( const T x, const T y ) MCLO_NOEXCEPT_TESTS
	{
		if ( T result; !detail::overflowing_div( x, y, result ) )
		{
			return result;
		}
		return std::nullopt;
	}
}
