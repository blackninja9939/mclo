#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <typename List>
		struct pop_first_impl;

		template <typename T, typename... Ts>
		struct pop_first_impl<type_list<T, Ts...>>
		{
			using type = type_list<Ts...>;
		};
	}

	template <typename List>
	using pop_first = typename detail::pop_first_impl<List>::type;
}
