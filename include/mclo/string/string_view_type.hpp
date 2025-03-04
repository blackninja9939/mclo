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

	template <typename String>
	using string_view_t = typename detail::string_view<String>::type;

	template <typename String>
	using string_char_t = typename string_view_t<String>::value_type;
}
