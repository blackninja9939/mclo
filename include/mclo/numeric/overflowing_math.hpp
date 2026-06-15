#pragma once

#include "mclo/numeric/detail/overflowing_intrinsics.hpp"

#include <utility>

namespace mclo
{
	template <standard_integral T>
	[[nodiscard]] constexpr std::pair<T, bool> overflowing_add( const T x, const T y ) noexcept
	{
		T result{};
		const bool overflowed = detail::overflowing_add( x, y, result );
		return { result, overflowed };
	}

	template <standard_integral T>
	[[nodiscard]] constexpr std::pair<T, bool> overflowing_sub( const T x, const T y ) noexcept
	{
		T result{};
		const bool overflowed = detail::overflowing_sub( x, y, result );
		return { result, overflowed };
	}

	template <standard_integral T>
	[[nodiscard]] constexpr std::pair<T, bool> overflowing_mul( const T x, const T y ) noexcept
	{
		T result{};
		const bool overflowed = detail::overflowing_mul( x, y, result );
		return { result, overflowed };
	}

	template <standard_integral T>
	[[nodiscard]] constexpr std::pair<T, bool> overflowing_div( const T x, const T y ) MCLO_NOEXCEPT_TESTS
	{
		T result{};
		const bool overflowed = detail::overflowing_div( x, y, result );
		return { result, overflowed };
	}
}
