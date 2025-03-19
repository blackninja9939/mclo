#pragma once

#include "mclo/numeric/detail/overflowing_intrinsics.hpp"

#include <optional>

namespace mclo
{
	template <standard_integral T>
	[[nodiscard]] constexpr std::optional<T> add_checked( const T x, const T y ) noexcept
	{
		if ( T result; !detail::add_overflow( x, y, result ) )
		{
			return result;
		}
		return std::nullopt;
	}

	template <standard_integral T>
	[[nodiscard]] constexpr std::optional<T> sub_checked( const T x, const T y ) noexcept
	{
		if ( T result; !detail::sub_overflow( x, y, result ) )
		{
			return result;
		}
		return std::nullopt;
	}

	template <standard_integral T>
	[[nodiscard]] constexpr std::optional<T> mul_checked( const T x, const T y ) noexcept
	{
		if ( T result; !detail::mul_overflow( x, y, result ) )
		{
			return result;
		}
		return std::nullopt;
	}

	template <standard_integral T>
	[[nodiscard]] constexpr std::optional<T> div_checked( const T x, const T y ) MCLO_NOEXCEPT_TESTS
	{
		if ( T result; !detail::div_overflow( x, y, result ) )
		{
			return result;
		}
		return std::nullopt;
	}
}
