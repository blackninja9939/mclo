#pragma once

#include <cstddef>
#include <functional>

namespace mclo
{
	namespace detail
	{
		[[nodiscard]] constexpr std::size_t hash_mix( std::size_t value ) noexcept
		{
			constexpr std::size_t constant = 0xe9846af9b1a615d;
			value ^= value >> 32;
			value *= constant;
			value ^= value >> 32;
			value *= constant;
			value ^= value >> 28;
			return value;
		}
	}

	template <typename... Ts>
	[[nodiscard]] std::size_t hash_combine( const Ts&... values ) noexcept
	{
		std::size_t hash = 0;
		( ( hash = detail::hash_mix( hash + 0x9e3779b9 + std::hash<Ts>()( values ) ) ), ... );
		return hash;
	}

	template <typename It>
	[[nodiscard]] std::size_t hash_range( It first, It last ) noexcept
	{
		using T = typename std::iterator_traits<It>::value_type;
		std::size_t hash = 0;
		while ( first != last )
		{
			hash = detail::hash_mix( hash + 0x9e3779b9 + std::hash<T>()( *first++ ) );
		}
		return hash;
	}
}
