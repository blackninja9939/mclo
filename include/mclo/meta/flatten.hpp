#pragma once

#include "join.hpp"
#include "transform.hpp"
#include "type_list.hpp"

#include <type_traits>

namespace mclo::meta
{
	namespace detail
	{
		template <typename T>
		using as_list = std::conditional<meta::is_list<T>, T, type_list<T>>;
	}

	/// @brief A @ref type_list with any nested @ref type_list elements of @p List spliced in one level deep.
	/// @details Non-list elements are kept as-is; each element that is itself a @ref type_list contributes its own
	/// elements. Flattening is applied a single level, not recursively.
	/// @tparam List The @ref type_list to flatten.
	template <typename List>
	using flatten = apply<detail::join_impl, transform<detail::as_list, List>>;
}
