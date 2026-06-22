#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <template <typename...> typename Predicate, typename List>
		struct any_of_impl;

		template <template <typename...> typename Predicate, typename... Ts>
		struct any_of_impl<Predicate, type_list<Ts...>> : std::bool_constant<( Predicate<Ts>::value || ... )>
		{
		};
	}

	/// @brief A @c std::bool_constant that is true if @p Predicate holds for at least one type in @p List.
	/// @details False for an empty list. @p Predicate is applied as @c Predicate<T>::value to each element.
	/// @tparam Predicate A class template whose @c value member gives the result for a single type.
	/// @tparam List The @ref type_list to test.
	template <template <typename...> typename Predicate, typename List>
	using any_of = typename detail::any_of_impl<Predicate, List>;

	/// @brief True if @p F holds for at least one type in @p List; see @ref any_of.
	/// @tparam F A class template whose @c value member gives the result for a single type.
	/// @tparam List The @ref type_list to test.
	template <template <typename...> typename F, typename List>
	constexpr auto any_of_v = any_of<F, List>::value;
}
