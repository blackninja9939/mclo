#pragma once

#include "mclo/numeric/detail/overflowing_intrinsics.hpp"

#include <optional>

namespace mclo
{
	template <standard_integral T>
	[[nodiscard]] constexpr std::optional<T> checked_add( const T x, const T y ) noexcept
	{
		if ( T result; !detail::overflowing_add( x, y, result ) )
		{
			return result;
		}
		return std::nullopt;
	}

	template <standard_integral T>
	[[nodiscard]] constexpr std::optional<T> checked_sub( const T x, const T y ) noexcept
	{
		if ( T result; !detail::overflowing_sub( x, y, result ) )
		{
			return result;
		}
		return std::nullopt;
	}

	template <standard_integral T>
	[[nodiscard]] constexpr std::optional<T> checked_mul( const T x, const T y ) noexcept
	{
		if ( T result; !detail::overflowing_mul( x, y, result ) )
		{
			return result;
		}
		return std::nullopt;
	}

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
