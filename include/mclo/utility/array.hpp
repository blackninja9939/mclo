#pragma once

#include "mclo/container/span.hpp"

#include <algorithm>
#include <array>
#include <type_traits>

namespace mclo
{
	namespace detail
	{
		template <std::size_t size, typename T, std::size_t... indices>
		[[nodiscard]] constexpr std::array<T, size> broadcast_array_internal(
			const T& value, std::index_sequence<indices...> ) noexcept( std::is_nothrow_copy_constructible_v<T> )
		{
			return { ( ( void )indices, value )... };
		}
	}

	/// @brief Creates a @c std::array of the given size with every element initialized to a copy of a value.
	/// @tparam size The number of elements in the resulting array.
	/// @tparam T The element type, deduced from @p value.
	/// @param value The value copied into every element of the array.
	/// @return A @c std::array<T, size> with each element equal to @p value.
	template <std::size_t size, typename T>
	[[nodiscard]] constexpr std::array<T, size> broadcast_array( const T& value ) noexcept(
		std::is_nothrow_copy_constructible_v<T> )
	{
		return detail::broadcast_array_internal<size>( value, std::make_index_sequence<size>{} );
	}

	namespace detail
	{
		template <typename OutputIt, typename T, std::size_t... size>
		constexpr void join_arrays_internal( OutputIt out, const std::array<T, size>&... arrays ) noexcept(
			std::is_nothrow_copy_assignable_v<T> )
		{
			( ( out = std::copy( arrays.begin(), arrays.end(), out ) ), ... );
		}
	}

	/// @brief Concatenates several @c std::array instances of the same element type into a single array.
	/// @tparam T The common element type of the arrays.
	/// @tparam size The sizes of each input array.
	/// @param arrays The arrays to concatenate, in order.
	/// @return A @c std::array<T, (size + ...)> containing the elements of each input array end to end.
	template <typename T, std::size_t... size>
	[[nodiscard]] constexpr auto join_arrays( const std::array<T, size>&... arrays ) noexcept(
		std::is_nothrow_default_constructible_v<T> && std::is_nothrow_copy_assignable_v<T> )
	{
		using result_type = std::array<T, ( size + ... )>;
		if ( std::is_constant_evaluated() )
		{
			result_type result{};
			detail::join_arrays_internal( result.begin(), arrays... );
			return result;
		}
		else
		{
			result_type result;
			detail::join_arrays_internal( result.begin(), arrays... );
			return result;
		}
	}

	namespace detail
	{
		template <typename T, std::size_t size, std::size_t... indices>
		[[nodiscard]] constexpr std::array<T, size> to_array_internal(
			const mclo::span<const T, size> data,
			std::index_sequence<indices...> ) noexcept( std::is_nothrow_copy_constructible_v<T> )
		{
			return { ( data[ indices ] )... };
		}
	}

	/// @brief Copies a fixed extent span into a @c std::array of the same size.
	/// @tparam T The element type of the span and resulting array.
	/// @tparam size The static extent of the span and size of the resulting array.
	/// @param data The span whose elements are copied into the array.
	/// @return A @c std::array<T, size> containing copies of the span's elements.
	template <typename T, std::size_t size>
	[[nodiscard]] constexpr std::array<T, size> to_array( const mclo::span<const T, size> data ) noexcept(
		std::is_nothrow_copy_constructible_v<T> )
	{
		return detail::to_array_internal( data, std::make_index_sequence<size>{} );
	}
}
