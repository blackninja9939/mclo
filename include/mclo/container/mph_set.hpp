#pragma once

#include "mclo/container/detail/mph_base.hpp"

namespace mclo
{
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
