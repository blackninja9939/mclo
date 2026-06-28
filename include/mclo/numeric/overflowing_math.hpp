#pragma once

#include "mclo/numeric/detail/overflowing_intrinsics.hpp"

#include <utility>

namespace mclo
{
	/// @brief Adds two integers, reporting whether the operation overflowed.
	/// @tparam T The integer type of the operands.
	/// @param x The first operand.
	/// @param y The second operand.
	/// @return A pair of the wrapped sum and a boolean that is @c true if the addition overflowed.
	template <standard_integral T>
	[[nodiscard]] constexpr std::pair<T, bool> overflowing_add( const T x, const T y ) noexcept
	{
		T result{};
		const bool overflowed = detail::overflowing_add( x, y, result );
		return { result, overflowed };
	}

	/// @brief Subtracts two integers, reporting whether the operation overflowed.
	/// @tparam T The integer type of the operands.
	/// @param x The value to subtract from.
	/// @param y The value to subtract.
	/// @return A pair of the wrapped difference and a boolean that is @c true if the subtraction overflowed.
	template <standard_integral T>
	[[nodiscard]] constexpr std::pair<T, bool> overflowing_sub( const T x, const T y ) noexcept
	{
		T result{};
		const bool overflowed = detail::overflowing_sub( x, y, result );
		return { result, overflowed };
	}

	/// @brief Multiplies two integers, reporting whether the operation overflowed.
	/// @tparam T The integer type of the operands.
	/// @param x The first operand.
	/// @param y The second operand.
	/// @return A pair of the wrapped product and a boolean that is @c true if the multiplication overflowed.
	template <standard_integral T>
	[[nodiscard]] constexpr std::pair<T, bool> overflowing_mul( const T x, const T y ) noexcept
	{
		T result{};
		const bool overflowed = detail::overflowing_mul( x, y, result );
		return { result, overflowed };
	}

	/// @brief Divides two integers, reporting whether the operation overflowed.
	/// @tparam T The integer type of the operands.
	/// @param x The dividend.
	/// @param y The divisor.
	/// @return A pair of the wrapped quotient and a boolean that is @c true if the division overflowed.
	/// @pre @p y must not be zero.
	template <standard_integral T>
	[[nodiscard]] constexpr std::pair<T, bool> overflowing_div( const T x, const T y ) noexcept
	{
		T result{};
		const bool overflowed = detail::overflowing_div( x, y, result );
		return { result, overflowed };
	}
}
