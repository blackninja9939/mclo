#pragma once

#include "mclo/strong_typedef/default_initialized.hpp"

namespace mclo
{
	/// @brief Marker mixin for semiregular value types: enables default construction and asserts the value type
	/// satisfies the std::semiregular concept.
	/// @details Derives from default_initialized, so supplying semiregular (or any mixin deriving from it, such as
	/// regular) also enables default construction.
	struct semiregular : default_initialized
	{
		template <typename Derived>
		struct mixin
		{
		};
	};
}
