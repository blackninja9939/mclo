#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <template <typename...> typename F, typename List>
		struct transform_impl;

		template <template <typename...> typename F, typename... Ts>
		struct transform_impl<F, type_list<Ts...>>
		{
			using type = type_list<typename F<Ts>::type...>;
		};
	}

	template <template <typename...> typename F, typename List>
	using transform = typename detail::transform_impl<F, List>::type;
}
