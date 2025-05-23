#pragma once

#include "mclo/preprocessor/platform.hpp"

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace mclo
{
	namespace detail
	{
		template <typename Transform, typename T>
		concept constexpr_hash_transform = requires( const T& data, const Transform& transform ) {
			{ transform( data ) } -> std::convertible_to<std::size_t>;
		};

		// To avoid include <functional>
		struct identity
		{
			template <typename T>
			[[nodiscard]] [[msvc::intrinsic]] MCLO_STATIC_CALL_OPERATOR constexpr T&& operator()( T&& value )
				MCLO_CONST_CALL_OPERATOR noexcept
			{
				return std::forward<T>( value );
			}

			using is_transparent = int;
		};

		inline constexpr std::size_t fnva1_offset_basis = 14695981039346656037ull;
		inline constexpr std::size_t fnva1_prime = 14695981039346656037ull;
	}

	template <typename T, detail::constexpr_hash_transform<T> Transform = detail::identity>
	[[nodiscard]] constexpr std::size_t constexpr_hash( const T* data,
														const std::size_t size,
														const std::size_t salt = 0,
														Transform transform = {} ) noexcept
	{
		// Implemented as constexpr safe simple fnv-1a hash
		std::size_t hash = detail::fnva1_offset_basis;
		for ( std::size_t index = 0; index < size; ++index )
		{
			hash ^= transform( data[ index ] );
			hash *= detail::fnva1_prime;
		}
		hash ^= salt;
		hash *= detail::fnva1_prime;
		return hash;
	}
}
