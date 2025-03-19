#pragma once

#include "mclo/numeric/detail/overflowing_intrinsics.hpp"

#include <utility>

namespace mclo
{
	template <standard_integral T>
	[[nodiscard]] constexpr std::pair<T, bool> add_overflowing( const T x, const T y ) noexcept
	{
		T result{};
		const bool overflowed = detail::add_overflow( x, y, result );
		return { result, overflowed };
	}

	template <standard_integral T>
	[[nodiscard]] constexpr std::pair<T, bool> sub_overflowing( const T x, const T y ) noexcept
	{
		T result{};
		const bool overflowed = detail::sub_overflow( x, y, result );
		return { result, overflowed };
	}

	template <standard_integral T>
	[[nodiscard]] constexpr std::pair<T, bool> mul_overflowing( const T x, const T y ) noexcept
	{
		T result{};
		const bool overflowed = detail::mul_overflow( x, y, result );
		return { result, overflowed };
	}

	template <standard_integral T>
	[[nodiscard]] constexpr std::pair<T, bool> div_overflowing( const T x, const T y ) MCLO_NOEXCEPT_TESTS
	{
		T result{};
		const bool overflowed = detail::div_overflow( x, y, result );
		return { result, overflowed };
	}
}
