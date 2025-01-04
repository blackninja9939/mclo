#pragma once

#include "mclo/hash/hasher.hpp"

#include <span>
#include <type_traits>

namespace mclo
{
	template <typename T>
	inline constexpr bool enable_bitwise_hash = std::is_integral_v<T> || std::is_enum_v<T> || std::is_pointer_v<T>;

	template <mclo::hasher Hasher, typename T>
		requires( enable_bitwise_hash<T> )
	void hash_append( Hasher& hasher, const T value ) noexcept
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
