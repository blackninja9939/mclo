#pragma once

#include <concepts>

namespace mclo
{
	/// @brief Requires that any type in Ts is the same as T
	template <typename T, typename... Ts>
	concept any_of_type = ( std::same_as<T, Ts> || ... );
}
