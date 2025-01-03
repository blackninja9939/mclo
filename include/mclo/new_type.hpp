#pragma once

#include "mclo/derived_from.hpp"
#include "mclo/platform.hpp"

#include <concepts>
#include <functional>
#include <type_traits>

namespace mclo
{
	template <typename Wrapped, typename Tag>
	class new_type
	{
	public:
		using value_type = Wrapped;

		value_type value;

		constexpr new_type() noexcept( std::is_nothrow_default_constructible_v<value_type> ) = default;

		constexpr explicit new_type( const value_type& raw ) noexcept(
			std::is_nothrow_copy_constructible_v<value_type> )
			: value( raw )
		{
		}

		constexpr explicit new_type( value_type&& raw ) noexcept( std::is_nothrow_move_constructible_v<value_type> )
			: value( std::move( raw ) )
		{
		}

		template <typename... Args>
		constexpr explicit new_type( std::in_place_t,
									 Args&&... args ) noexcept( std::is_nothrow_constructible_v<value_type, Args...> )
			: value( std::forward<Args>( args )... )
		{
		}

		constexpr operator const value_type&() const noexcept
		{
			return value;
		}
		constexpr operator value_type&() noexcept
		{
			return value;
		}

		constexpr friend void swap( new_type& lhs, new_type& rhs ) noexcept( std::is_nothrow_swappable_v<value_type> )
		{
			using std::swap;
			swap( lhs.value, rhs.value );
		}

		constexpr auto operator<=>( const new_type& rhs ) const noexcept = default;
	};
}

namespace std
{
	template <typename T, typename Tag>
	struct hash<mclo::new_type<T, Tag>>
	{
		MCLO_STATIC_CALL_OPERATOR std::size_t operator()( const mclo::new_type<T, Tag>& object )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			return std::hash<T>{}( object.value );
		}
	};
}
