#pragma once

#include <string>
#include <string_view>

namespace mclo
{
	/// @brief Converts a narrow (UTF-8 / system encoding) string to a wide string.
	/// @param str The narrow string to convert.
	/// @return The converted wide string.
	std::wstring to_wstring( const std::string_view str );
}
