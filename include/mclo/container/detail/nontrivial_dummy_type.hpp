#pragma once

#include <type_traits>

namespace mclo::detail
{
	struct nontrivial_dummy_type
	{
		constexpr nontrivial_dummy_type() noexcept
		{
			// This default constructor is user-provided to avoid zero-initialization when objects are
			// value-initialized.
		}
	};
	static_assert( !std::is_trivially_default_constructible_v<nontrivial_dummy_type> );
}
