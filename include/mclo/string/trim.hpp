#pragma once

#include "mclo/string/ascii_string_utils.hpp"
#include "mclo/string/string_view_type.hpp"

#include <string_view>

namespace mclo
{
	template <typename CharT>
	constexpr auto whitespace_characters_v = trandscode_ascii_literal<CharT>( " \n\f\t\r\v" );

	template <typename CharT>
	constexpr auto numeric_characters_v = trandscode_ascii_literal<CharT>( "0123456789" );

	template <typename CharT>
	constexpr auto uppercase_characters_v = trandscode_ascii_literal<CharT>( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" );

	template <typename CharT>
	constexpr auto lowercase_characters_v = trandscode_ascii_literal<CharT>( "abcdefgihjklmnopqrstuvwxyz" );

	template <typename CharT>
	constexpr auto alphabet_characters_v =
		trandscode_ascii_literal<CharT>( "abcdefgihjklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" );

	template <typename CharT>
	constexpr auto alphanumeric_characters_v =
		trandscode_ascii_literal<CharT>( "abcdefgihjklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" );

	inline constexpr auto whitespace_characters = whitespace_characters_v<char>;
	inline constexpr auto numeric_characters = numeric_characters_v<char>;
	inline constexpr auto uppercase_characters = uppercase_characters_v<char>;
	inline constexpr auto lowercase_characters = lowercase_characters_v<char>;
	inline constexpr auto alphabet_characters = alphabet_characters_v<char>;
	inline constexpr auto alphanumeric_characters = alphanumeric_characters_v<char>;

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
