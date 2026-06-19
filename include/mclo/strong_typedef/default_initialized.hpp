#pragma once

namespace mclo
{
	/// @brief Marker mixin that enables default construction of a strong_typedef.
	/// @details Supplying this mixin, or any mixin deriving from it such as semiregular or regular, gives the
	/// strong_typedef a default constructor provided the underlying value_type is itself default constructible.
	struct default_initialized
	{
		template <typename Derived>
		struct mixin
		{
		};
	};
}
