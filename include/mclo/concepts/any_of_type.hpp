#pragma once

#include <concepts>

namespace mclo
{
	template <typename T, typename... Ts>
	concept any_of_type = ( std::same_as<T, Ts> || ... );
}
