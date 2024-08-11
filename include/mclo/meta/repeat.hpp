#pragma once

#include "type_list.hpp"

#include <utility>

namespace mclo::meta
{
	namespace detail
	{
		template <typename Indices, typename List>
		struct repeat_impl;

		template <std::size_t... Indices, typename List>
		struct repeat_impl<std::index_sequence<Indices...>, List>
		{
			using type = meta::join<std::decay_t<decltype( Indices, std::declval<List>() )>...>;
		};
	}

	template <std::size_t Amount, typename List>
	using repeat = typename detail::repeat_impl<std::make_index_sequence<Amount>, List>::type;
}
