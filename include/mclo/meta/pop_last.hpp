#pragma once

#include "nth_type.hpp"
#include "type_list.hpp"

#include <utility>

namespace mclo::meta
{
	namespace detail
	{
		template <typename List>
		struct pop_last_impl;

		template <typename... Ts>
		struct pop_last_impl<type_list<Ts...>>
		{
			using List = type_list<Ts...>;

			template <std::size_t... Indices>
			static type_list<mclo::meta::nth<Indices, List>...> extract( std::index_sequence<Indices...> );

			using type = decltype( extract( std::make_index_sequence<sizeof...( Ts ) - 1>{} ) );
		};
	}

	template <typename List>
	using pop_last = typename detail::pop_last_impl<List>::type;
}
