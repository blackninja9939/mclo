#pragma once

#include "type_list.hpp"

#include <type_traits>

namespace mclo::meta
{
	namespace detail
	{
		template <typename T, std::size_t Index, typename List>
		struct index_of_impl;

		template <typename T, std::size_t Index>
		struct index_of_impl<T, Index, type_list<>>
		{
		};

		template <typename T, std::size_t Index, typename Head, typename... Ts>
		struct index_of_impl<T, Index, type_list<Head, Ts...>> : index_of_impl<T, Index + 1, type_list<Ts...>>
		{
		};

		template <typename T, std::size_t Index, typename Head, typename... Ts>
			requires( std::is_same_v<T, Head> )
		struct index_of_impl<T, Index, type_list<Head, Ts...>> : std::integral_constant<std::size_t, Index>
		{
		};
	}

	template <typename T, typename List>
	using index_of = typename detail::index_of_impl<T, 0, List>;

	template <typename T, typename List>
	constexpr auto index_of_v = index_of<T, List>::value;
}
