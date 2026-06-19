#pragma once

#include <utility>

namespace mclo
{
	/// @brief Mixin that adds an implicit conversion to the underlying value_type.
	/// @details By default a strong_typedef only exposes its value via the public value member. This mixin opts back
	/// into the implicit conversion of the wrapped value, trading type safety for convenience.
	struct implicitly_convertible
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] constexpr operator const auto &() const noexcept
			{
				return static_cast<const Derived&>( *this ).value;
			}
			[[nodiscard]] constexpr operator auto &() noexcept
			{
				return static_cast<Derived&>( *this ).value;
			}
		};
	};

	/// @brief Parameterised mixin that adds an implicit conversion to a specific target type.
	/// @details Unlike implicitly_convertible, which exposes the underlying value_type, this converts the wrapped value
	/// to Target via static_cast, allowing a single chosen implicit conversion.
	/// @tparam Target The type to implicitly convert to.
	template <typename Target>
	struct implicitly_convertible_to
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] constexpr operator Target() const
				noexcept( noexcept( static_cast<Target>( std::declval<const Derived&>().value ) ) )
			{
				return static_cast<Target>( static_cast<const Derived&>( *this ).value );
			}
		};
	};
}
