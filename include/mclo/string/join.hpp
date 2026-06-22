#pragma once

#include "mclo/string/concatenate.hpp"
#include "mclo/string/string_view_type.hpp"

#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace mclo
{
	/// @brief Joins the elements of the range [@p first, @p last) into a string separated by @p delimter.
	/// @details Each element is converted as by @ref append_string, so non-string elements such as numbers are
	/// formatted automatically. The result is pre-sized when the iterators form a forward range of string-like values.
	/// @tparam String The string type to build.
	/// @param first Iterator to the first element.
	/// @param last Sentinel marking the end of the range.
	/// @param delimter The separator inserted between consecutive elements.
	/// @return A @p String containing the joined elements.
	template <typename String = std::string, std::input_iterator It, std::sentinel_for<It> Sentinel>
	[[nodiscard]] constexpr String join_string( It first, Sentinel last, const std::string_view delimter )
	{
		String result;

		// Forward range of things directly convertible to string we can pre-size the result
		if constexpr ( std::forward_iterator<It> && std::convertible_to<std::string_view, std::iter_reference_t<It>> )
		{
			auto it = first;
			using size_type = typename String::size_type;
			size_type size = 0;
			while ( it != last )
			{
				size += std::string_view( *it++ ).size();
				size += delimter.size();
			}

			result.reserve( size );
		}

		std::string_view delimiter_to_write;
		while ( first != last )
		{
			result.append( delimiter_to_write );
			append_string( result, *first++ );
			delimiter_to_write = delimter;
		}
		return result;
	}

	/// @brief Joins the elements of @p range into a string separated by @p delimter.
	/// @details Each element is converted as by @ref append_string, so non-string elements such as numbers are
	/// formatted automatically.
	/// @tparam String The string type to build.
	/// @param range The range of elements to join.
	/// @param delimter The separator inserted between consecutive elements.
	/// @return A @p String containing the joined elements.
	template <typename String = std::string, std::ranges::input_range Range>
	[[nodiscard]] constexpr String join_string( Range&& range, const std::string_view delimter )
	{
		return join_string<String>( std::ranges::begin( range ), std::ranges::end( range ), delimter );
	}
}
