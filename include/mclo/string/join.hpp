#pragma once

#include "mclo/string/concatenate.hpp"
#include "mclo/string/string_view_type.hpp"

#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace mclo
{
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

	template <typename String = std::string, std::ranges::input_range Range>
	[[nodiscard]] constexpr String join_string( Range&& range, const std::string_view delimter )
	{
		return join_string<String>( std::ranges::begin( range ), std::ranges::end( range ), delimter );
	}
}
