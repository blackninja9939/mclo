#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <template <typename...> typename Predicate, typename List>
		struct any_of_impl;

		template <template <typename...> typename Predicate, typename... Ts>
		struct any_of_impl<Predicate, type_list<Ts...>> : std::bool_constant<( Predicate<Ts>::value || ... )>
		{
		};
	}

	template <template <typename...> typename Predicate, typename List>
	using any_of = typename detail::any_of_impl<Predicate, List>;

	template <template <typename...> typename F, typename List>
	constexpr auto any_of_v = any_of<F, List>::value;
}
