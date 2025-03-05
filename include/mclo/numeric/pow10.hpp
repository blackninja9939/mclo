#pragma once

#include "mclo/debug/assert.hpp"

#include <cstddef>

namespace mclo
{
	namespace detail
	{
		// More than this overflows std::size_t max
		inline constexpr std::size_t pow10s[] = {
			1ull,
			10ull,
			100ull,
			1000ull,
			10000ull,
			100000ull,
			1000000ull,
			10000000ull,
			100000000ull,
			1000000000ull,
			10000000000ull,
			100000000000ull,
			1000000000000ull,
			10000000000000ull,
			100000000000000ull,
			1000000000000000ull,
			10000000000000000ull,
			100000000000000000ull,
			1000000000000000000ull,
			10000000000000000000ull,
		};

		// To avoid including it via iterator and other large headers
		template <typename T, std::size_t N>
		[[nodiscard]] constexpr std::size_t size( const T ( & )[ N ] ) noexcept
		{
			return N;
		}
	}

	[[nodiscard]] constexpr std::size_t pow10( const unsigned char exponent ) MCLO_NOEXCEPT_TESTS
	{
		DEBUG_ASSERT( exponent < detail::size( detail::pow10s ), "Result would overflow std::size_t" );
		return detail::pow10s[ exponent ];
	}
}
