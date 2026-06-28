#pragma once

#include "mclo/debug/assert.hpp"

#include <array>
#include <string_view>

namespace mclo
{
	/// @brief A compile-time fixed-length string of exactly @p N characters, usable as a non-type template parameter.
	/// @details Stores its characters inline in a @c std::array, with the length fixed by the template parameter rather
	/// than tracked at runtime. Primarily used to carry string contents through templates, for example in
	/// @ref mclo::meta::type_name_v.
	/// @tparam N The number of characters in the string.
	template <std::size_t N>
	class fixed_string
	{
	public:
		/// @brief Constructs from @p str, which must be exactly @p N characters long.
		/// @param str The source characters to copy.
		/// @pre @p str must have a size of @p N.
		constexpr explicit fixed_string( const std::string_view str ) noexcept
		{
			DEBUG_ASSERT( str.size() == N, "String size does not match fixed string size" );
			for ( size_t i = 0; i < N; ++i )
			{
				m_data[ i ] = str[ i ];
			}
		}

		/// @brief Converts to a @c std::string_view over the stored characters.
		[[nodiscard]] constexpr operator std::string_view() const noexcept
		{
			return std::string_view( m_data.data(), N );
		}

	private:
		std::array<char, N> m_data{};
	};
}
