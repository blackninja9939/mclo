#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <ranges>

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

	template <typename T>
	concept is_nothrow_hashable = requires( const T& value, const std::hash<T> hasher ) {
		{
			hasher( value )
		} noexcept -> std::convertible_to<std::size_t>;
	};

	template <typename... Ts>
	[[nodiscard]] constexpr std::size_t hash_combine( const Ts&... values ) noexcept( ( is_nothrow_hashable<Ts> &&
																						... ) )
	{
		std::size_t hash = 0;
		( ( hash = detail::hash_mix( hash + 0x9e3779b9 + std::hash<Ts>()( values ) ) ), ... );
		return hash;
	}

	template <std::input_iterator It, std::sentinel_for<It> Sentinel>
	[[nodiscard]] constexpr std::size_t hash_range( It first, Sentinel last ) noexcept(
		is_nothrow_hashable<std::iter_reference_t<It>> )
	{
		using hasher = std::hash<std::iter_value_t<It>>;
		std::size_t hash = 0;
		while ( first != last )
		{
			hash = detail::hash_mix( hash + 0x9e3779b9 + hasher()( *first++ ) );
		}
		return hash;
	}

	template <std::ranges::input_range Range>
	[[nodiscard]] constexpr std::size_t hash_range( Range&& range ) noexcept(
		is_nothrow_hashable<std::ranges::range_value_t<Range>> )
	{
		return hash_range( std::ranges::begin( range ), std::ranges::end( range ) );
	}
}
