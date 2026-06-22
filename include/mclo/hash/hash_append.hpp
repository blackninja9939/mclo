#pragma once

#include "mclo/hash/hasher.hpp"

#include <type_traits>

namespace mclo
{
	/// @brief Feeds @p value into @p hasher by hashing its raw object bytes.
	/// @details The customization point of the hashing framework: overload @c hash_append for a type to make it
	/// hashable. This default applies to types with a unique object representation, hashing the bytes directly. User
	/// types should provide their own @c hash_append overload found by argument-dependent lookup.
	/// @param hasher The hasher to write into.
	/// @param value The value to hash.
	template <mclo::hasher Hasher, typename T>
		requires( std::has_unique_object_representations_v<T> )
	void hash_append( Hasher& hasher, const T& value ) noexcept
	{
		hasher.write( { reinterpret_cast<const std::byte*>( &value ), sizeof( T ) } );
	}

	/// @brief Feeds a null pointer into @p hasher.
	template <mclo::hasher Hasher>
	void hash_append( Hasher& hasher, const std::nullptr_t ) noexcept
	{
		constexpr const void* value = nullptr;
		hash_append( hasher, value );
	}

	/// @brief Concept satisfied when @p T can be hashed into @p Hasher via a @c noexcept @ref hash_append.
	/// @tparam T The type to hash.
	/// @tparam Hasher The hasher type.
	template <typename T, typename Hasher>
	concept hashable_with = requires( const T& value, Hasher& h ) {
		requires hasher<Hasher>;
		{ hash_append( h, value ) } noexcept;
	};
}
