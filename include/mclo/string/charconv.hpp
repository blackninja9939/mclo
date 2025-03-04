#pragma once

#include <array>
#include <charconv>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>

namespace mclo
{
	template <typename T, typename... FromCharArgs>
	[[nodiscard]] std::optional<T> from_string( const std::string_view str, FromCharArgs&&... args ) noexcept
	{
		T value;
		const std::from_chars_result result =
			std::from_chars( str.data(), str.data() + str.size(), value, std::forward<FromCharArgs>( args )... );

		if ( result.ec == std::errc{} )
		{
			return value;
		}
		return {};
	}

	template <typename T, typename... ToCharArgs>
	[[nodiscard]] std::string_view to_string( char* const first,
											  char* const last,
											  const T value,
											  ToCharArgs&&... args ) noexcept
	{
		const auto [ ptr, ec ] = std::to_chars( first, last, value, std::forward<ToCharArgs>( args )... );

		if ( ec == std::errc{} )
		{
			return std::string_view( first, ptr - first );
		}
		return {};
	}

	template <std::ranges::output_range<char> Range, typename T, typename... ToCharArgs>
		requires( std::ranges::contiguous_range<Range> )
	[[nodiscard]] std::string_view to_string( Range&& buffer, const T value, ToCharArgs&&... args ) noexcept
	{
		return to_string( std::ranges::data( buffer ),
						  std::ranges::data( buffer ) + std::ranges::size( buffer ),
						  value,
						  std::forward<ToCharArgs>( args )... );
	}
}
