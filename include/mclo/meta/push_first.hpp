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

	template <typename T, typename List>
	using push_first = typename detail::push_first_impl<T, List>::type;
}
