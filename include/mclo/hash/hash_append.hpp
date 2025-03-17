#pragma once

#include "mclo/hash/hasher.hpp"

#include <span>
#include <type_traits>

namespace mclo
{
	template <mclo::hasher Hasher, typename T>
		requires( std::has_unique_object_representations_v<T> )
	void hash_append( Hasher& hasher, const T& value ) noexcept
	{
		hasher.write( { reinterpret_cast<const std::byte*>( &value ), sizeof( T ) } );
	}

	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::nullptr_t ) noexcept
	{
		constexpr const void* value = nullptr;
		hash_append( hasher, value );
	}

	template <typename T, typename Hasher>
	concept hashable_with = requires( const T& value, Hasher& h ) {
		requires hasher<Hasher>;
		{ hash_append( h, value ) } noexcept;
	};
}
