#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace mclo
{
	namespace detail
	{
		template <typename T, typename Tuple, std::size_t... Indices>
		[[nodiscard]] T* allocate_from_tuple( Tuple&& tuple, std::index_sequence<Indices...> )
		{
			// construct T from the elements of tuple
			static_assert( std::is_constructible_v<T, decltype( std::get<Indices>( std::forward<Tuple>( tuple ) ) )...>,
						   "the target type must be constructible from the fields of the argument tuple." );
			return new T( std::get<Indices>( std::forward<Tuple>( tuple ) )... );
		}
	}

	/// @brief Allocates a @p T on the heap, constructing it from the elements of a tuple.
	/// @details Unpacks @p tuple and forwards its elements as constructor arguments to a new @c T, analogous to
	/// @c std::make_from_tuple but heap allocating with @c new.
	/// @tparam T The type to allocate and construct. Must be constructible from the tuple's elements.
	/// @tparam Tuple The tuple-like type holding the constructor arguments.
	/// @param tuple The tuple whose elements are forwarded to the constructor of @p T.
	/// @return A pointer to the newly allocated @p T. The caller owns it and must @c delete it.
	template <typename T, typename Tuple>
	[[nodiscard]] T* allocate_from_tuple( Tuple&& tuple )
	{
		return detail::allocate_from_tuple<T>(
			std::forward<Tuple>( tuple ),
			std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>{} );
	}
}
