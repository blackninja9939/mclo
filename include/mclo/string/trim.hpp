#pragma once

#include "mclo/string/ascii_string_utils.hpp"
#include "mclo/string/string_view_type.hpp"

#include <string_view>

namespace mclo
{
	/// @brief The ASCII whitespace characters, as a buffer of @p CharT.
	template <typename CharT>
	constexpr auto whitespace_characters_v = transcode_ascii_literal<CharT>( " \n\f\t\r\v" );

	/// @brief The decimal digit characters, as a buffer of @p CharT.
	template <typename CharT>
	constexpr auto numeric_characters_v = transcode_ascii_literal<CharT>( "0123456789" );

	/// @brief The uppercase ASCII letters, as a buffer of @p CharT.
	template <typename CharT>
	constexpr auto uppercase_characters_v = transcode_ascii_literal<CharT>( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" );

	/// @brief The lowercase ASCII letters, as a buffer of @p CharT.
	template <typename CharT>
	constexpr auto lowercase_characters_v = transcode_ascii_literal<CharT>( "abcdefghijklmnopqrstuvwxyz" );

	/// @brief The ASCII letters of both cases, as a buffer of @p CharT.
	template <typename CharT>
	constexpr auto alphabet_characters_v =
		transcode_ascii_literal<CharT>( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" );

	/// @brief The ASCII letters and decimal digits, as a buffer of @p CharT.
	template <typename CharT>
	constexpr auto alphanumeric_characters_v =
		transcode_ascii_literal<CharT>( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" );

	/// @brief The ASCII whitespace characters as a @c char buffer.
	inline constexpr auto whitespace_characters = whitespace_characters_v<char>;
	/// @brief The decimal digit characters as a @c char buffer.
	inline constexpr auto numeric_characters = numeric_characters_v<char>;
	/// @brief The uppercase ASCII letters as a @c char buffer.
	inline constexpr auto uppercase_characters = uppercase_characters_v<char>;
	/// @brief The lowercase ASCII letters as a @c char buffer.
	inline constexpr auto lowercase_characters = lowercase_characters_v<char>;
	/// @brief The ASCII letters of both cases as a @c char buffer.
	inline constexpr auto alphabet_characters = alphabet_characters_v<char>;
	/// @brief The ASCII letters and decimal digits as a @c char buffer.
	inline constexpr auto alphanumeric_characters = alphanumeric_characters_v<char>;

	/// @brief Returns a view of @p string with leading @p to_trim characters removed.
	/// @param string The string-like value to trim.
	/// @param to_trim The set of characters to strip from the front; defaults to whitespace.
	/// @return A view of the remaining string, or an empty view if every character was trimmed.
	template <typename String>
	[[nodiscard]] constexpr string_view_t<String> trim_front(
		const String& string,
		const string_view_t<String> to_trim = whitespace_characters_v<string_char_t<String>> ) noexcept
	{
		const string_view_t<String> view{ string };
		const auto start = view.find_first_not_of( to_trim );
		if ( start == string_view_t<String>::npos )
		{
			return {};
		}
		return view.substr( start );
	}

	/// @brief Returns a view of @p string with trailing @p to_trim characters removed.
	/// @param string The string-like value to trim.
	/// @param to_trim The set of characters to strip from the back; defaults to whitespace.
	/// @return A view of the remaining string, or an empty view if every character was trimmed.
	template <typename String>
	[[nodiscard]] constexpr string_view_t<String> trim_back(
		const String& string,
		const string_view_t<String> to_trim = whitespace_characters_v<string_char_t<String>> ) noexcept
	{
		const string_view_t<String> view{ string };
		const auto end = view.find_last_not_of( to_trim );
		if ( end == string_view_t<String>::npos )
		{
			return {};
		}
		return view.substr( 0, end + 1 );
	}

	/// @brief Returns a view of @p string with both leading and trailing @p to_trim characters removed.
	/// @param string The string-like value to trim.
	/// @param to_trim The set of characters to strip from both ends; defaults to whitespace.
	/// @return A view of the remaining string, or an empty view if every character was trimmed.
	template <typename String>
	[[nodiscard]] constexpr string_view_t<String> trim(
		const String& string,
		const string_view_t<String> to_trim = whitespace_characters_v<string_char_t<String>> ) noexcept
	{
		const string_view_t<String> view{ string };
		auto start = view.find_first_not_of( to_trim );
		if ( start == string_view_t<String>::npos )
		{
			return {};
		}

		const auto end = view.find_last_not_of( to_trim );
		if ( end == string_view_t<String>::npos )
		{
			return view.substr( start );
		}

		return view.substr( start, end + 1 - start );
	}
}
