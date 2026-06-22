#pragma once

#include "mclo/container/detail/mph_base.hpp"

namespace mclo
{
	/// @brief A fixed-size, immutable set built on a minimal perfect hash, ideal for compile-time constant tables.
	/// @details Stores a fixed set of @p Size values known at construction and supports collision-free membership
	/// queries in at most two hashes and one comparison. The whole set can be a @c constexpr value. See
	/// @ref detail::mph_base for the underlying lookup mechanism and @ref mph_hash for customising the value hash.
	/// @warning Building the perfect hash is a fairly heavy compile-time computation (repeatedly searching for salts).
	/// It is constant-evaluation capable but can noticeably hurt compile times if used for many or large tables, so
	/// use it judiciously.
	/// @tparam Value The element type.
	/// @tparam Size The exact number of elements.
	/// @tparam Hash The salted hash functor for values.
	/// @tparam KeyEquals The value equality comparator.
	template <typename Value,
			  std::size_t Size,
			  typename Hash = mph_hash<Value>,
			  typename KeyEquals = std::equal_to<Value>>
	class mph_set : public detail::mph_base<Value, Value, Hash, KeyEquals, std::identity, Size>
	{
		using base = detail::mph_base<Value, Value, Hash, KeyEquals, std::identity, Size>;

	public:
		using base::base;
	};
}
