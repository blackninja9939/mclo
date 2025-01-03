#pragma once

#include "mclo/hash/hash.hpp"

namespace mclo
{
	template <hasher Hasher = default_hasher, hashable_with<Hasher> T>
	struct hasher_for
	{
		std::size_t operator()( const T& value ) const noexcept
		{
			return hash<Hasher>( value );
		}
	};
}
