#pragma once

#include "mclo/utility/index_constant.hpp"

#include <utility>

namespace mclo
{
	namespace detail
	{
		template <std::size_t... Indices, typename Func>
		constexpr void loop_unroll( Func func,
									std::index_sequence<Indices...> ) noexcept( std::is_nothrow_invocable_v<Func> )
		{
			( func( index_constant<Indices>{} ), ... );
		}
	}

	template <std::size_t Count, typename Func>
	constexpr void loop_unroll( Func func ) noexcept( std::is_nothrow_invocable_v<Func> )
	{
		detail::loop_unroll( func, std::make_index_sequence<Count>{} );
	}
}
