#pragma once

#include <type_traits>

namespace mclo
{
	template <typename T, typename... Ts>
	constexpr bool is_any_of_v = ( std::is_same_v<T, Ts> || ... );
}
