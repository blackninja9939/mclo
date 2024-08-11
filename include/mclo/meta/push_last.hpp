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

	template <typename T, typename List>
	using push_last = typename detail::push_last_impl<T, List>::type;
}
