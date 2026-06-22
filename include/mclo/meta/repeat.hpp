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

	/// @brief A @ref type_list formed by concatenating @p List with itself @p Amount times.
	/// @details An @p Amount of zero yields an empty @ref type_list.
	/// @tparam Amount The number of times to repeat @p List.
	/// @tparam List The @ref type_list to repeat.
	template <std::size_t Amount, typename List>
	using repeat = typename detail::repeat_impl<std::make_index_sequence<Amount>, List>::type;
}
