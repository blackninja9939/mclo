#pragma once

#include <type_traits>

namespace mclo
{
	/// @brief Converts an enumeration to its underlying integer type.
	/// @details Mirrors @c std::to_underlying from C++23, allowing the value of an enumerator to be obtained as its
	/// underlying type without an explicit and error prone @c static_cast.
	/// @tparam Enum The enumeration type.
	/// @param value The enumeration value to convert.
	/// @return @p value cast to @c std::underlying_type_t<Enum>.
	template <typename Enum>
	[[nodiscard]] constexpr std::underlying_type_t<Enum> to_underlying( const Enum value ) noexcept
	{
		return static_cast<std::underlying_type_t<Enum>>( value );
	}
}
