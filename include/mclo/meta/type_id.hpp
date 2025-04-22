#pragma once

namespace mclo::meta
{
	namespace detail
	{
		template <typename T>
		struct type_id
		{
			static constexpr int value = 0;
		};
	}

	using type_id_t = const void*;

	template <typename T>
	constexpr type_id_t type_id = &detail::type_id<T>::value;
}
