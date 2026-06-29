#pragma once

#include <utility>

namespace mclo::strong_type
{
	/// @brief Mixin that adds an explicit conversion to bool based on the truthiness of the wrapped value.
	struct boolean
	{
		template <typename Derived>
		struct mixin
		{
			[[nodiscard]] constexpr explicit operator bool() const
				noexcept( noexcept( static_cast<bool>( std::declval<const Derived&>().value ) ) )
			{
				return static_cast<bool>( static_cast<const Derived&>( *this ).value );
			}
		};
	};
}
