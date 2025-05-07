#pragma once

#include "type_id.hpp"

#include <array>

namespace mclo::meta
{
	template <typename... Ts>
	struct type_list
	{
		static constexpr std::size_t size = sizeof...( Ts );
		static constexpr std::array<type_id_t, size> ids = { type_id<Ts>... };
	};

	template<typename List>
	constexpr bool is_list = false;

	template <typename... Ts>
	constexpr bool is_list<type_list<Ts...>> = true;

	template <typename List>
	constexpr bool empty = List::size == 0;
}
