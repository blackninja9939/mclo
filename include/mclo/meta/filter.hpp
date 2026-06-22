#pragma once

#include "join.hpp"
#include "type_list.hpp"

#include <type_traits>

namespace mclo::meta
{
	namespace detail
	{
		template <template <typename...> typename Predicate, typename List>
		struct filter_impl;

		template <template <typename...> typename Predicate, typename... Ts>
		struct filter_impl<Predicate, type_list<Ts...>>
		{
			using type = join<std::conditional_t<Predicate<Ts>::value, type_list<Ts>, type_list<>>...>;
		};
	}

	/// @brief A @ref type_list containing only the elements of @p List for which @p Predicate holds.
	/// @details Order is preserved. @p Predicate is applied as @c Predicate<T>::value to each element.
	/// @tparam Predicate A class template whose @c value member decides whether to keep a type.
	/// @tparam List The @ref type_list to filter.
	template <template <typename...> typename Predicate, typename List>
	using filter = typename detail::filter_impl<Predicate, List>::type;
}
