#pragma once

#include "mclo/strong_type/type.hpp"

#include "mclo/platform/cpp_feature_compat.hpp"

#include <cstddef>
#include <functional>

namespace mclo::strong_type
{
	/// @brief Mixin that opts a strong type into hashing.
	/// @details Enables the std::hash specialization, delegating to the underlying value's hash.
	struct hashable
	{
		template <typename Derived>
		struct mixin
		{
		};
	};
}

namespace std
{
	template <typename Wrapped, typename Tag, typename... Mixins>
		requires mclo::strong_type::
			has_mixin<mclo::strong_type::type<Wrapped, Tag, Mixins...>, mclo::strong_type::hashable>
		struct hash<mclo::strong_type::type<Wrapped, Tag, Mixins...>>
	{
		MCLO_STATIC_CALL_OPERATOR std::size_t operator()(
			const mclo::strong_type::type<Wrapped, Tag, Mixins...>& object
		) MCLO_CONST_CALL_OPERATOR noexcept
		{
			return std::hash<Wrapped>{}( object.value );
		}
	};
}
