#pragma once

#include "algorithm.hpp"
#include "constant_evaluated.hpp"

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
			( ( out = mclo::copy( arrays.begin(), arrays.end(), out ) ), ... );
		}
	}

	template <typename T, std::size_t... size>
	[[nodiscard]] constexpr auto join_arrays( const std::array<T, size>&... arrays ) noexcept(
		std::is_nothrow_default_constructible_v<T> && std::is_nothrow_copy_assignable_v<T> )
	{
		using result_type = std::array<T, ( size + ... )>;
		if ( mclo::is_constant_evaluated() )
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
}
