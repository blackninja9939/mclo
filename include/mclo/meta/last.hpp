#pragma once

#include "type_list.hpp"

#include <type_traits>

namespace mclo::meta
{
	namespace detail
	{
		template <typename List>
		struct last_impl;

		template <typename... Ts>
		struct last_impl<type_list<Ts...>>
		{
			using type = typename decltype( ( std::type_identity<Ts>{}, ... ) )::type;
		};
	}

	/// @brief The last type in @p List.
	/// @tparam List A non-empty @ref type_list.
	template <typename List>
	using last = typename detail::last_impl<List>::type;
}
