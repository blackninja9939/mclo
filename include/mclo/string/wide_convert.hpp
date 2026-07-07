#pragma once

#include <string>
#include <string_view>

namespace mclo
{
	/// @brief Converts a narrow (UTF-8 / system encoding) string to a wide string.
	/// @param str The narrow string to convert.
	/// @return The converted wide string.
	std::wstring to_wstring( const std::string_view str );

	/// @brief Converts a wide string to a narrow (UTF-8 / system encoding) string.
	/// @param str The wide string to convert.
	/// @return The converted narrow string.
	std::string from_wstring( const std::wstring_view str );
}
