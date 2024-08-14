#pragma once

#include "mclo/platform.hpp"

#include <cstddef>
#include <type_traits>

namespace mclo
{
	namespace detail
	{
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
	}

	template <typename T, typename Transform = detail::identity>
	[[nodiscard]] constexpr std::size_t fnv1a( const T* data,
											   const std::size_t size,
											   Transform transform = {} ) noexcept
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
