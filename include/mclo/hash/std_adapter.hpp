#pragma once

#include "mclo/hash/hash.hpp"

#include "mclo/preprocessor/platform.hpp"

namespace mclo
{
	template <hashable_with<default_hasher> T>
	struct std_hash_adapter
	{
		[[nodiscard]] MCLO_STATIC_CALL_OPERATOR std::size_t operator()( const T& value )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			return hash<default_hasher>( value );
		}
	};
}
