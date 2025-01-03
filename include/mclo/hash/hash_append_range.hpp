#pragma once

#include "mclo/hash/hash_append.hpp"

#include <concepts>
#include <ranges>

namespace mclo
{
	template <hasher Hasher, std::ranges::forward_range Range>
		requires( hashable_with<std::ranges::range_value_t<Range>, Hasher> )
	void hash_append_range( Hasher& hasher, Range&& range ) noexcept
	{
		if constexpr ( std::ranges::contiguous_range<Range> && enable_bitwise_hash<std::ranges::range_value_t<Range>> )
		{
			hasher.write( std::as_bytes( std::span( std::forward<Range>( range ) ) ) );
		}
		else
		{
			for ( auto&& value : range )
			{
				hash_append( hasher, value );
			}
		}
	}
}
