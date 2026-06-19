#pragma once

#include "mclo/strong_typedef/strong_typedef.hpp"

#include "mclo/hash/hash_append.hpp"
#include "mclo/platform/cpp_feature_compat.hpp"

#include <cstddef>
#include <functional>

namespace mclo
{
	/// @brief Mixin that opts a strong_typedef into hashing.
	/// @details Provides an ADL hash_append for the mclo hashing framework and enables the std::hash specialization,
	/// both delegating to the underlying value's hash.
	struct hashable
	{
		template <typename Derived>
		struct mixin
		{
			template <mclo::hasher Hasher>
			friend void hash_append( Hasher& hasher, const Derived& object ) noexcept
			{
				using mclo::hash_append;
				hash_append( hasher, object.value );
			}
		};
	};
}

namespace std
{
	template <typename Wrapped, typename Tag, typename... Mixins>
		requires mclo::has_mixin<mclo::strong_typedef<Wrapped, Tag, Mixins...>, mclo::hashable>
	struct hash<mclo::strong_typedef<Wrapped, Tag, Mixins...>>
	{
		MCLO_STATIC_CALL_OPERATOR std::size_t operator()( const mclo::strong_typedef<Wrapped, Tag, Mixins...>& object )
			MCLO_CONST_CALL_OPERATOR noexcept
		{
			return std::hash<Wrapped>{}( object.value );
		}
	};
}
