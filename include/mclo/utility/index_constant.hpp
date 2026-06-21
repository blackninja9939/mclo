#pragma once

#include <type_traits>

namespace mclo
{
	/// @brief A compile time @c std::size_t value, modelled as an @c std::integral_constant.
	/// @details Useful for passing indices as types, for example in fold expressions or to tag dispatch on a
	/// particular index, as done by @ref loop_unroll.
	/// @tparam Value The index value carried by the constant.
	template <std::size_t Value>
	using index_constant = std::integral_constant<std::size_t, Value>;
}
