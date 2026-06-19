#pragma once

#include <cstddef>
#include <memory>

namespace mclo
{
	/// @brief Mixin that adds pointer-like access (operator*, operator->) and comparison against nullptr.
	/// @details Intended for strong_typedefs whose value_type is a raw or smart pointer. The arrow operator yields the
	/// underlying raw pointer via std::to_address.
	struct pointer
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] constexpr decltype( auto ) operator*() const
			{
				return *static_cast<const Derived&>( *this ).value;
			}
			[[nodiscard]] constexpr decltype( auto ) operator*()
			{
				return *static_cast<Derived&>( *this ).value;
			}
			[[nodiscard]] constexpr auto operator->() const
			{
				return std::to_address( static_cast<const Derived&>( *this ).value );
			}
			[[nodiscard]] constexpr auto operator->()
			{
				return std::to_address( static_cast<Derived&>( *this ).value );
			}
			[[nodiscard]] friend constexpr bool operator==( const Derived& lhs,
															std::nullptr_t ) noexcept( noexcept( lhs.value ==
																								 nullptr ) )
			{
				return lhs.value == nullptr;
			}
		};
	};
}
