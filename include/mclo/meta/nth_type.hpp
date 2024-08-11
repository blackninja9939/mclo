#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <std::size_t Index, typename List>
		struct nth_impl;

		template <std::size_t Index>
		struct nth_impl<Index, type_list<>>
		{
			static_assert( mclo::always_false<std::integral_constant<std::size_t, Index>>, "Empty type list" );
		};

		template <typename T, typename... Remaining>
		struct nth_impl<0, type_list<T, Remaining...>>
		{
			using type = T;
		};

		template <std::size_t Index, typename T, typename... Remaining>
		struct nth_impl<Index, type_list<T, Remaining...>> : nth_impl<Index - 1, type_list<Remaining...>>
		{
		};
	}

	template <std::size_t Index, typename List>
	using nth = typename detail::nth_impl<Index, List>::type;
}
