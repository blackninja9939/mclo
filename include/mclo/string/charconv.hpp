#pragma once

#include <array>
#include <charconv>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>

namespace mclo
{
	/// @brief Parses a value of type @p T from @p str using @c std::from_chars.
	/// @details A thin wrapper returning an empty optional instead of an error code on failure. The whole string need
	/// not be consumed; parsing stops at the first character that does not form part of the value.
	/// @tparam T The arithmetic type to parse.
	/// @param str The text to parse.
	/// @param args Extra arguments forwarded to @c std::from_chars (such as a base or @c std::chars_format).
	/// @return The parsed value, or @c std::nullopt if no value could be parsed.
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

	/// @brief Formats @p value into the buffer [@p first, @p last) using @c std::to_chars.
	/// @tparam T The arithmetic type to format.
	/// @param first Pointer to the start of the output buffer.
	/// @param last Pointer past the end of the output buffer.
	/// @param value The value to format.
	/// @param args Extra arguments forwarded to @c std::to_chars (such as a base, @c std::chars_format, or precision).
	/// @return A view of the written characters, or an empty view if the buffer was too small.
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

	/// @brief Formats @p value into the contiguous output range @p buffer using @c std::to_chars.
	/// @tparam T The arithmetic type to format.
	/// @param buffer The contiguous character buffer to write into.
	/// @param value The value to format.
	/// @param args Extra arguments forwarded to @c std::to_chars.
	/// @return A view of the written characters, or an empty view if the buffer was too small.
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
