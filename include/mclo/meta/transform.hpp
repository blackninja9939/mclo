#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <template <typename...> typename F, typename List>
		struct transform_impl;

		template <template <typename...> typename F, typename... Ts>
		struct transform_impl<F, type_list<Ts...>>
		{
			using type = type_list<typename F<Ts>::type...>;
		};
	}

	/// @brief A @ref type_list with @p F applied to each element of @p List, yielding @c F<T>::type per element.
	/// @tparam F A class template providing a @c type member, applied to each element individually.
	/// @tparam List The @ref type_list to transform.
	template <template <typename...> typename F, typename List>
	using transform = typename detail::transform_impl<F, List>::type;
}
