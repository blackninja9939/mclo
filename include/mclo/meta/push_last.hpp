#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <typename T, typename List>
		struct push_last_impl;

		template <typename T, typename... Ts>
		struct push_last_impl<T, type_list<Ts...>>
		{
			using type = type_list<Ts..., T>;
		};
	}

	/// @brief A @ref type_list equal to @p List with @p T appended as the new last element.
	/// @tparam T The type to add to the back.
	/// @tparam List The @ref type_list to append to.
	template <typename T, typename List>
	using push_last = typename detail::push_last_impl<T, List>::type;
}
