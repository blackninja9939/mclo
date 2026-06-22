#pragma once

#include <string>
#include <string_view>
#include <type_traits>

namespace mclo
{
	/// @brief Replaces every non-overlapping occurrence of @p find in @p string with @p replace, in place.
	/// @details Does nothing if @p find is empty. Replacement text is not rescanned, so a @p replace that contains
	/// @p find will not cause further substitutions.
	/// @param string The string to modify.
	/// @param find The substring to search for.
	/// @param replace The text to substitute for each occurrence of @p find.
	template <typename CharT>
	void replace_all( std::basic_string<CharT>& string,
					  const std::basic_string_view<std::type_identity_t<CharT>> find,
					  const std::basic_string_view<std::type_identity_t<CharT>> replace ) noexcept
	{
		const std::size_t find_size = find.size();
		if ( find_size == 0 )
		{
			return;
		}

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
}
