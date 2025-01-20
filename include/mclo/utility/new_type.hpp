#pragma once

#include "mclo/hash/hash_append.hpp"
#include "mclo/preprocessor/platform.hpp"

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

		[[nodiscard]] constexpr auto operator<=>( const new_type& rhs ) const noexcept = default;
	};

	template <typename T, typename Tag>
	constexpr bool enable_bitwise_hash<new_type<T, Tag>> = enable_bitwise_hash<T>;

	template <hasher Hasher, typename T, typename Tag>
		requires( !enable_bitwise_hash<T> )
	void hash_append( Hasher& hasher, const new_type<T, Tag>& object ) noexcept
	{
		hash_append( hasher, object.value );
	}
}

namespace std
{
	// Specifically doesn't use hash_append because it's a new type and the value should be hashed directly like its std
	// counterpart
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
