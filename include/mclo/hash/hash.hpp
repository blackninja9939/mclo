#pragma once

#include "mclo/hash/default_hasher.hpp"
#include "mclo/hash/hash_append.hpp"
#include "mclo/hash/hash_append_range.hpp"

#include "mclo/preprocessor/platform.hpp"

namespace mclo
{
	template <typename T, hasher Hasher = mclo::default_hasher>
		requires( hashable_with<T, Hasher> )
	struct hash
	{
		[[nodiscard]] std::size_t operator()( const T& value ) const noexcept
		{
			Hasher local = m_hasher;
			hash_append( local, value );
			return local.finish();
		}

		MCLO_NO_UNIQUE_ADDRESS Hasher m_hasher;
	};

	template <hasher Hasher = mclo::default_hasher, hashable_with<Hasher> T>
	std::size_t hash_object( const T& value ) noexcept
	{
		Hasher h;
		hash_append( h, value );
		return h.finish();
	}

	template <hasher Hasher = mclo::default_hasher, std::ranges::forward_range Range>
		requires( hashable_with<std::ranges::range_value_t<Range>, Hasher> )
	std::size_t hash_range( Range&& range ) noexcept
	{
		Hasher h;
		hash_append_range( h, std::forward<Range>( range ) );
		return h.finish();
	}

	template <typename T>
	concept default_hashable = hashable_with<T, mclo::default_hasher>;
}
