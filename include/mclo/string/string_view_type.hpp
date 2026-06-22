#pragma once

#include <string_view>

namespace mclo
{
	namespace detail
	{
		template <typename String>
		struct string_view
		{
			using type = std::basic_string_view<typename String::value_type, typename String::traits_type>;
		};

		template <typename CharT>
		struct string_view<CharT*>
		{
			using type = std::basic_string_view<CharT>;
		};

		template <typename CharT>
		struct string_view<const CharT*>
		{
			using type = std::basic_string_view<CharT>;
		};

		template <typename CharT, std::size_t N>
		struct string_view<const CharT[ N ]>
		{
			using type = std::basic_string_view<CharT>;
		};

		template <typename CharT, std::size_t N>
		struct string_view<CharT[ N ]>
		{
			using type = std::basic_string_view<CharT>;
		};
	}

	/// @brief The @c std::basic_string_view type corresponding to the string-like type @p String.
	/// @details Maps owning strings, string views, character pointers, and character arrays to the matching
	/// @c std::basic_string_view, letting generic code accept any string-like input.
	/// @tparam String A string, string view, character pointer, or character array type.
	template <typename String>
	using string_view_t = typename detail::string_view<String>::type;

	/// @brief The character type of the string-like type @p String.
	/// @tparam String A string, string view, character pointer, or character array type.
	template <typename String>
	using string_char_t = typename string_view_t<String>::value_type;
}
