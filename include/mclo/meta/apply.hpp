#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <template <typename...> typename F, typename List>
		struct apply_impl;

		template <template <typename...> typename F, typename... Ts>
		struct apply_impl<F, type_list<Ts...>>
		{
			using type = typename F<Ts...>::type;
		};
	}

	/// @brief Instantiates the metafunction @p F with the types of @p List as its arguments, yielding @c
	/// F<Ts...>::type.
	/// @details The inverse of holding types in a @ref type_list: it unpacks the list back into @p F's template
	/// arguments.
	/// @tparam F A class template providing a @c type member when instantiated with the list's elements.
	/// @tparam List The @ref type_list whose elements become @p F's arguments.
	template <template <typename...> typename F, typename List>
	using apply = typename detail::apply_impl<F, List>::type;

	/// @brief The @c value member of @c F<Ts...> for the elements of @p List; see @ref apply.
	/// @tparam F A class template providing a @c value member when instantiated with the list's elements.
	/// @tparam List The @ref type_list whose elements become @p F's arguments.
	template <template <typename...> typename F, typename List>
	constexpr auto apply_v = apply<F, List>::value;
}
