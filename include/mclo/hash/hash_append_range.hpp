#pragma once

#include "mclo/hash/hash_append.hpp"

#include <concepts>
#include <ranges>

namespace mclo
{
	/// @brief Feeds every element of @p range into @p hasher.
	/// @details The number of elements is hashed first so that ranges of differing length, or differently-split
	/// sequences of nested ranges, produce distinct hashes. For a contiguous range of trivially-byte-comparable
	/// elements the whole buffer is then hashed at once; otherwise each element is hashed individually via
	/// @ref hash_append.
	/// @param hasher The hasher to write into.
	/// @param range The range of elements to hash.
	template <hasher Hasher, std::ranges::forward_range Range>
		requires( hashable_with<std::ranges::range_value_t<Range>, Hasher> )
	void hash_append_range( Hasher& hasher, Range&& range ) noexcept
	{
		hash_append( hasher, static_cast<std::size_t>( std::ranges::distance( range ) ) );
		if constexpr ( std::ranges::contiguous_range<Range> &&
					   std::has_unique_object_representations_v<std::ranges::range_value_t<Range>> )
		{
			hasher.write( mclo::as_bytes( mclo::span( std::forward<Range>( range ) ) ) );
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
