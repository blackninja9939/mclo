#pragma once

#include "mclo/strong_typedef/equality_comparable.hpp"
#include "mclo/strong_typedef/semiregular.hpp"

#include "mclo/platform/attributes.hpp"

namespace mclo
{
	/// @brief Preset bundle for regular value-like strong types: semiregular plus equality comparison.
	struct regular : semiregular
	{
		template <typename Derived>
		struct MCLO_EMPTY_BASES mixin : semiregular::mixin<Derived>, equality_comparable::mixin<Derived>
		{
		};
	};
}
