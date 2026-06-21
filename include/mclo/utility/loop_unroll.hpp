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

	/// @brief Invokes a callable @p Count times, passing each iteration's index as a compile time constant.
	/// @details Expands to a fold expression rather than a runtime loop, so the index is available as a
	/// @ref index_constant template argument and each iteration can be specialized at compile time.
	/// @tparam Count The number of iterations to perform.
	/// @tparam Func A callable invocable with an @ref index_constant.
	/// @param func The callable invoked once per index with @c index_constant<I>{}.
	template <std::size_t Count, typename Func>
	constexpr void loop_unroll( Func func ) noexcept( std::is_nothrow_invocable_v<Func> )
	{
		detail::loop_unroll( func, std::make_index_sequence<Count>{} );
	}
}
