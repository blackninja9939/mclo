#pragma once

#include "identity.hpp"

#include <cstddef>
#include <type_traits>

namespace mclo
{
	template <typename T, typename Transform = identity>
	[[nodiscard]] constexpr std::size_t fnv1a( const T* data, const std::size_t size, Transform transform = {} ) noexcept
	{
		static_assert( std::is_convertible_v<decltype( transform( *data ) ), std::size_t>,
					   "Transform result of data must be convertible to std::size_t" );
		std::size_t hash = 14695981039346656037ull; // FNV Offset basis
		for ( std::size_t index = 0; index < size; ++index )
		{
			hash ^= transform( data[ index ] );
			hash *= 1099511628211ull; // FNV Prime
		}
		return hash;
	}
}
