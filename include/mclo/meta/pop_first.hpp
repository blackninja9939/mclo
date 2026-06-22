#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <typename List>
		struct pop_first_impl;

		template <typename T, typename... Ts>
		struct pop_first_impl<type_list<T, Ts...>>
		{
			using type = type_list<Ts...>;
		};
	}

	/// @brief A @ref type_list equal to @p List with its first element removed.
	/// @tparam List A non-empty @ref type_list.
	template <typename List>
	using pop_first = typename detail::pop_first_impl<List>::type;
}
