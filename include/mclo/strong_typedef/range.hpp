#pragma once

#include <ranges>
#include <utility>

namespace mclo
{
	/// @brief Mixin exposing the wrapped value as a range by forwarding to the std::ranges customization points.
	/// @details Generates begin() and end() (with const overloads) that dispatch to std::ranges::begin and
	/// std::ranges::end on the wrapped value, so the strong type itself models a range usable in range-for loops and
	/// the standard ranges algorithms. Each accessor is constrained on the wrapped value being a range, so the strong
	/// type models std::ranges::range exactly when the underlying value does. When the wrapped value is a
	/// std::ranges::sized_range, size() and empty() are additionally provided, dispatching to std::ranges::size and
	/// std::ranges::empty.
	struct range
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] constexpr auto begin() noexcept(
				noexcept( std::ranges::begin( std::declval<typename Derived::value_type&>() ) ) )
				requires std::ranges::range<typename Derived::value_type>
			{
				return std::ranges::begin( static_cast<Derived&>( *this ).value );
			}

			[[nodiscard]] constexpr auto begin() const
				noexcept( noexcept( std::ranges::begin( std::declval<const typename Derived::value_type&>() ) ) )
				requires std::ranges::range<const typename Derived::value_type>
			{
				return std::ranges::begin( static_cast<const Derived&>( *this ).value );
			}

			[[nodiscard]] constexpr auto end() noexcept(
				noexcept( std::ranges::end( std::declval<typename Derived::value_type&>() ) ) )
				requires std::ranges::range<typename Derived::value_type>
			{
				return std::ranges::end( static_cast<Derived&>( *this ).value );
			}

			[[nodiscard]] constexpr auto end() const
				noexcept( noexcept( std::ranges::end( std::declval<const typename Derived::value_type&>() ) ) )
				requires std::ranges::range<const typename Derived::value_type>
			{
				return std::ranges::end( static_cast<const Derived&>( *this ).value );
			}

			[[nodiscard]] constexpr auto size() const
				noexcept( noexcept( std::ranges::size( std::declval<const typename Derived::value_type&>() ) ) )
				requires std::ranges::sized_range<const typename Derived::value_type>
			{
				return std::ranges::size( static_cast<const Derived&>( *this ).value );
			}

			[[nodiscard]] constexpr bool empty() const
				noexcept( noexcept( std::ranges::empty( std::declval<const typename Derived::value_type&>() ) ) )
				requires std::ranges::sized_range<const typename Derived::value_type>
			{
				return std::ranges::empty( static_cast<const Derived&>( *this ).value );
			}
		};
	};
}
