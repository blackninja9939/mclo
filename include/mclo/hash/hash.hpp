#pragma once

#include "mclo/hash/default_hasher.hpp"
#include "mclo/hash/hash_append.hpp"
#include "mclo/hash/hash_append_range.hpp"

namespace mclo
{
	template <hasher Hasher = mclo::default_hasher, hashable_with<Hasher> T>
	std::size_t hash( const T& value ) noexcept
	{
		Hasher hasher;
		hash_append( hasher, value );
		return hasher.finish();
	}

	template <hasher Hasher = mclo::default_hasher, std::ranges::forward_range Range>
		requires( hashable_with<std::ranges::range_value_t<Range>, Hasher> )
	std::size_t hash_range( Range&& range ) noexcept
	{
		Hasher hasher;
		hash_append_range( hasher, std::forward<Range>( range ) );
		return hasher.finish();
	}

	template <typename T>
	concept default_hashable = hashable_with<T, mclo::default_hasher>;
}
