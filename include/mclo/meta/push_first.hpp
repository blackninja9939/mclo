#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <typename T, typename List>
		struct push_first_impl;

		template <typename T, typename... Ts>
		struct push_first_impl<T, type_list<Ts...>>
		{
			using type = type_list<T, Ts...>;
		};
	}

	/// @brief A @ref type_list equal to @p List with @p T prepended as the new first element.
	/// @tparam T The type to add to the front.
	/// @tparam List The @ref type_list to prepend to.
	template <typename T, typename List>
	using push_first = typename detail::push_first_impl<T, List>::type;
}
