#pragma once

#include "join.hpp"
#include "type_list.hpp"

#include <type_traits>

namespace mclo::meta
{
	namespace detail
	{
		template <template <typename...> typename Predicate, typename List>
		struct filter_impl;

		template <template <typename...> typename Predicate, typename... Ts>
		struct filter_impl<Predicate, type_list<Ts...>>
		{
			using type = join<std::conditional_t<Predicate<Ts>::value, type_list<Ts>, type_list<>>...>;
		};
	}

	template <template <typename...> typename Predicate, typename List>
	using filter = typename detail::filter_impl<Predicate, List>::type;
}
