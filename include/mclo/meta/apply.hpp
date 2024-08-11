#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <template <typename...> typename F, typename List>
		struct apply_impl;

		template <template <typename...> typename F, typename... Ts>
		struct apply_impl<F, type_list<Ts...>>
		{
			using type = typename F<Ts...>::type;
		};
	}

	template <template <typename...> typename F, typename List>
	using apply = typename detail::apply_impl<F, List>::type;

	template <template <typename...> typename F, typename List>
	constexpr auto apply_v = apply<F, List>::value;
}
