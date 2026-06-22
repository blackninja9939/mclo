#pragma once

#include "type_list.hpp"

namespace mclo::meta
{
	namespace detail
	{
		template <template <typename...> typename Predicate, typename List>
		struct all_of_impl;

		template <template <typename...> typename Predicate, typename... Ts>
		struct all_of_impl<Predicate, type_list<Ts...>> : std::bool_constant<( Predicate<Ts>::value && ... )>
		{
		};
	}

	/// @brief A @c std::bool_constant that is true if @p Predicate holds for every type in @p List.
	/// @details Vacuously true for an empty list. @p Predicate is applied as @c Predicate<T>::value to each element.
	/// @tparam Predicate A class template whose @c value member gives the result for a single type.
	/// @tparam List The @ref type_list to test.
	template <template <typename...> typename Predicate, typename List>
	using all_of = typename detail::all_of_impl<Predicate, List>;

	/// @brief True if @p F holds for every type in @p List; see @ref all_of.
	/// @tparam F A class template whose @c value member gives the result for a single type.
	/// @tparam List The @ref type_list to test.
	template <template <typename...> typename F, typename List>
	constexpr auto all_of_v = all_of<F, List>::value;
}
