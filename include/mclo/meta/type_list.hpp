#pragma once

#include "type_id.hpp"

#include <array>

namespace mclo::meta
{
	/// @brief A compile-time list of types, the central data structure of the @c mclo::meta library.
	/// @details Holds a parameter pack of types and exposes their count and per-type @ref type_id values. The various
	/// @c mclo::meta algorithms (such as @ref nth, @ref filter, @ref transform, @ref join) operate on @c type_list.
	/// @tparam Ts The types contained in the list.
	template <typename... Ts>
	struct type_list
	{
		/// @brief The number of types in the list.
		static constexpr std::size_t size = sizeof...( Ts );
		/// @brief The @ref type_id of each type in the list, in order.
		static constexpr std::array<type_id_t, size> ids = { type_id<Ts>... };
	};

	/// @brief True if @p List is a specialization of @ref type_list.
	/// @tparam List The type to test.
	template <typename List>
	constexpr bool is_list = false;

	template <typename... Ts>
	constexpr bool is_list<type_list<Ts...>> = true;

	/// @brief True if @p List contains no types.
	/// @tparam List The @ref type_list to test.
	template <typename List>
	constexpr bool empty = List::size == 0;
}
