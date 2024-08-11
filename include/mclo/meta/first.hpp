#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <typename List>
		struct first_impl;

		template <typename T, typename... Ts>
		struct first_impl<type_list<T, Ts...>>
		{
			using type = T;
		};
	}

	template <typename List>
	using first = typename detail::first_impl<List>::type;
}
