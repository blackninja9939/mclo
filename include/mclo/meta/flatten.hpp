#pragma once

#include "join.hpp"
#include "transform.hpp"
#include "type_list.hpp"

#include <type_traits>

namespace mclo::meta
{
	namespace detail
	{
		template <typename T>
		using as_list = std::conditional<meta::is_list<T>, T, type_list<T>>;
	}

	template <typename List>
	using flatten = apply<detail::join_impl, transform<detail::as_list, List>>;
}
