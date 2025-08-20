#pragma once

#include <type_traits>

namespace mclo
{
	template <std::size_t Value>
	using index_constant = std::integral_constant<std::size_t, Value>;
}
