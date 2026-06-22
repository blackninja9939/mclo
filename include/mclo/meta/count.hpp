#pragma once

#include "type_list.hpp"

#include <type_traits>

namespace mclo::meta
{
	namespace detail
	{
		template <typename T, std::size_t Index, typename List>
		struct count_impl;

		template <typename T, std::size_t Index>
		struct count_impl<T, Index, type_list<>> : std::integral_constant<std::size_t, Index>
		{
		};

		template <typename T, std::size_t Index, typename Head, typename... Ts>
		struct count_impl<T, Index, type_list<Head, Ts...>>
			: count_impl<T, Index + std::is_same_v<T, Head>, type_list<Ts...>>
		{
		};
	}

	/// @brief The number of times @p T appears in @p List, as a @c std::integral_constant.
	/// @tparam T The type to count.
	/// @tparam List The @ref type_list to search.
	template <typename T, typename List>
	using count = typename detail::count_impl<T, 0, List>;

	/// @brief The number of times @p T appears in @p List as a @c std::size_t value.
	/// @tparam T The type to count.
	/// @tparam List The @ref type_list to search.
	template <typename T, typename List>
	constexpr auto count_v = count<T, List>::value;
}
