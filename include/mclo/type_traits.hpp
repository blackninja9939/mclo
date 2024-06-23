#pragma once

#include <type_traits>

namespace mclo
{
#ifdef __cpp_lib_type_identity
	using std::type_identity;
	using std::type_identity_t;
#else
	template <typename T>
	struct type_identity
	{
		using type = T;
	};

	template <typename T>
	using type_identity_t = typename type_identity<T>::type;
#endif

	template <typename... Ts>
	constexpr bool always_false = false;
}
