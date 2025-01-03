#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <template <typename...> typename Predicate, typename List>
		struct all_of_impl;

		template <template <typename...> typename Predicate, typename... Ts>
		struct all_of_impl<Predicate, type_list<Ts...>> : std::bool_constant<( Predicate<Ts>::value && ... )>
		{
		};
	}

	template <template <typename...> typename Predicate, typename List>
	using all_of = typename detail::all_of_impl<Predicate, List>;

	template <template <typename...> typename F, typename List>
	constexpr auto all_of_v = all_of<F, List>::value;
}
