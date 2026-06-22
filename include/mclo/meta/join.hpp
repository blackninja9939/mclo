#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <typename... Lists>
		struct join_impl
		{
			using type = type_list<>;
		};

		template <typename List>
		struct join_impl<List>
		{
			using type = List;
		};

		template <typename... Lhs, typename... Rhs>
		struct join_impl<type_list<Lhs...>, type_list<Rhs...>>
		{
			using type = type_list<Lhs..., Rhs...>;
		};

		template <typename... Lhs, typename... Rhs, typename... Lists>
		struct join_impl<type_list<Lhs...>, type_list<Rhs...>, Lists...>
		{
			using type = typename join_impl<type_list<Lhs..., Rhs...>, Lists...>::type;
		};
	}

	/// @brief A single @ref type_list concatenating the elements of all the given @ref type_list arguments in order.
	/// @details Joining no lists yields an empty @ref type_list.
	/// @tparam Lists The @ref type_list specializations to concatenate.
	template <typename... Lists>
	using join = typename detail::join_impl<Lists...>::type;
}
