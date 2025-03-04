#pragma once

#include "mclo/preprocessor/platform.hpp"

#include "ascii_string_utils.hpp"
#include "string_view_type.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace mclo
{
	template <typename CharT>
	void replace_all( std::basic_string<CharT>& string,
					  const std::basic_string_view<std::type_identity_t<CharT>> find,
					  const std::basic_string_view<std::type_identity_t<CharT>> replace ) noexcept
	{
		const std::size_t find_size = find.size();
		const std::size_t replace_size = replace.size();
		std::size_t index = 0;
		for ( ;; )
		{
			index = string.find( find, index );
			if ( index == std::basic_string<CharT>::npos )
			{
				break;
			}
			string.replace( index, find_size, replace );
			index += replace_size;
		}
	}

	void to_upper( std::wstring& string ) noexcept;
	void to_lower( std::wstring& string ) noexcept;
}
