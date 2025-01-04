#pragma once

#include "mclo/preprocessor/platform.hpp"

#include <tuple>
#include <type_traits>
#include <utility>

namespace mclo
{
	namespace detail
	{
		template <typename T, typename Tuple, std::size_t... Indices>
		[[nodiscard]] T* allocate_from_tuple( Tuple&& tuple, std::index_sequence<Indices...> ) noexcept(
			std::is_nothrow_constructible_v<T, decltype( std::get<Indices>( std::forward<Tuple>( tuple ) ) )...> )
		{
			// construct T from the elements of tuple
			static_assert( std::is_constructible_v<T, decltype( std::get<Indices>( std::forward<Tuple>( tuple ) ) )...>,
						   "the target type must be constructible from the fields of the argument tuple." );
			return new T( std::get<Indices>( std::forward<Tuple>( tuple ) )... );
		}
	}

	template <typename T, typename Tuple>
	[[nodiscard]] T* allocate_from_tuple( Tuple&& tuple ) MCLO_NOEXCEPT_AND_BODY( detail::allocate_from_tuple<T>(
		std::forward<Tuple>( tuple ), std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{} ) )
}
