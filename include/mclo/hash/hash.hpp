#pragma once

#include "mclo/hash/default_hasher.hpp"
#include "mclo/hash/hash_append.hpp"
#include "mclo/hash/hash_append_range.hpp"

#include "mclo/platform/attributes.hpp"

namespace mclo
{
	/// @brief A hash functor compatible with the standard library's hash-based containers.
	/// @details Hashes a value of type @p T using @p Hasher via @ref hash_append, mirroring the interface of
	/// @c std::hash. A copy of the stored hasher is taken per call so any seed state is preserved across invocations.
	/// @tparam T The type to hash.
	/// @tparam Hasher The hasher algorithm to use, defaulting to @ref default_hasher.
	template <typename T, hasher Hasher = mclo::default_hasher>
		requires( hashable_with<T, Hasher> && std::is_copy_constructible_v<Hasher> )
	struct hash
	{
		/// @brief Computes the hash of @p value.
		/// @param value The value to hash.
		/// @return The combined hash result.
		[[nodiscard]] std::size_t operator()( const T& value ) const
			noexcept( std::is_nothrow_copy_constructible_v<Hasher> )
		{
			Hasher local = m_hasher;
			hash_append( local, value );
			return local.finish();
		}

		/// @brief The hasher prototype copied for each hash operation, allowing a custom seed.
		MCLO_NO_UNIQUE_ADDRESS Hasher m_hasher;
	};

	/// @brief Computes the hash of a single object using a freshly constructed hasher.
	/// @tparam Hasher The hasher algorithm to use, defaulting to @ref default_hasher.
	/// @param value The value to hash.
	/// @param args Arguments forwarded to the hasher's constructor, e.g. a seed.
	/// @return The hash result.
	template <hasher Hasher = mclo::default_hasher, hashable_with<Hasher> T, typename... Args>
		requires( std::is_constructible_v<Hasher, Args...> )
	std::size_t hash_object( const T& value,
							 Args&&... args ) noexcept( std::is_nothrow_constructible_v<Hasher, Args...> )
	{
		Hasher h( std::forward<Args>( args )... );
		hash_append( h, value );
		return h.finish();
	}

	/// @brief Computes the hash of a range of elements using a freshly constructed hasher.
	/// @tparam Hasher The hasher algorithm to use, defaulting to @ref default_hasher.
	/// @param range The range of elements to hash.
	/// @param args Arguments forwarded to the hasher's constructor, e.g. a seed.
	/// @return The hash result.
	/// @see hash_append_range
	template <hasher Hasher = mclo::default_hasher, std::ranges::forward_range Range, typename... Args>
		requires( hashable_with<std::ranges::range_value_t<Range>, Hasher> && std::is_constructible_v<Hasher, Args...> )
	std::size_t hash_range( Range&& range, Args&&... args ) noexcept( std::is_nothrow_constructible_v<Hasher, Args...> )
	{
		Hasher h( std::forward<Args>( args )... );
		hash_append_range( h, std::forward<Range>( range ) );
		return h.finish();
	}

	/// @brief Concept satisfied when @p T can be hashed using the library's @ref default_hasher.
	/// @tparam T The type to test.
	template <typename T>
	concept default_hashable = hashable_with<T, mclo::default_hasher>;
}
