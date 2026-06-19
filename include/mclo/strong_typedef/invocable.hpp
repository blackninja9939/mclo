#pragma once

#include <utility>

namespace mclo
{
	/// @brief Mixin that adds a call operator forwarding to the wrapped value.
	/// @details Intended for strong_typedefs whose value_type is itself callable. Arguments are perfectly forwarded.
	struct invocable
	{
		template <typename Derived>
		struct mixin
		{
			template <typename... Args>
			constexpr decltype( auto ) operator()( Args&&... args ) const
				noexcept( noexcept( std::declval<const Derived&>().value( std::declval<Args>()... ) ) )
			{
				return static_cast<const Derived&>( *this ).value( std::forward<Args>( args )... );
			}
			template <typename... Args>
			constexpr decltype( auto ) operator()( Args&&... args ) noexcept(
				noexcept( std::declval<Derived&>().value( std::declval<Args>()... ) ) )
			{
				return static_cast<Derived&>( *this ).value( std::forward<Args>( args )... );
			}
		};
	};
}
