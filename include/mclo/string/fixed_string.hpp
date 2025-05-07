#pragma once

#include "mclo/debug/assert.hpp"

#include <array>
#include <string_view>

namespace mclo
{
	template <std::size_t N>
	class fixed_string
	{
	public:
		constexpr explicit fixed_string( const std::string_view str ) MCLO_NOEXCEPT_TESTS
		{
			DEBUG_ASSERT( str.size() == N, "String size does not match fixed string size" );
			for ( size_t i = 0; i < N; ++i )
			{
				m_data[ i ] = str[ i ];
			}
		}

		[[nodiscard]] constexpr operator std::string_view() const noexcept
		{
			return std::string_view( m_data.data(), N );
		}

	private:
		std::array<char, N> m_data{};
	};
}
