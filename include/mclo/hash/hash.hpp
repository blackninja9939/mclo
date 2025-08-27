#pragma once

#include "mclo/hash/default_hasher.hpp"
#include "mclo/hash/hash_append.hpp"
#include "mclo/hash/hash_append_range.hpp"

#include "mclo/preprocessor/platform.hpp"

namespace mclo
{
	template <typename T, hasher Hasher = mclo::default_hasher>
		requires( hashable_with<T, Hasher> && std::is_copy_constructible_v<Hasher> )
	struct hash
	{
		[[nodiscard]] std::size_t operator()( const T& value ) const
			noexcept( std::is_nothrow_copy_constructible_v<Hasher> )
		{
			Hasher local = m_hasher;
			hash_append( local, value );
			return local.finish();
		}

		MCLO_NO_UNIQUE_ADDRESS Hasher m_hasher;
	};

	template <hasher Hasher = mclo::default_hasher, hashable_with<Hasher> T>
		requires( std::is_default_constructible_v<Hasher> )
	std::size_t hash_object( const T& value ) noexcept( std::is_nothrow_default_constructible_v<Hasher> )
	{
		Hasher h;
		hash_append( h, value );
		return h.finish();
	}

	template <hasher Hasher = mclo::default_hasher, std::ranges::forward_range Range>
		requires( hashable_with<std::ranges::range_value_t<Range>, Hasher> && std::is_default_constructible_v<Hasher> )
	std::size_t hash_range( Range&& range ) noexcept( std::is_nothrow_default_constructible_v<Hasher> )
	{
		Hasher h;
		hash_append_range( h, std::forward<Range>( range ) );
		return h.finish();
	}

	template <typename T>
	concept default_hashable = hashable_with<T, mclo::default_hasher>;
}
