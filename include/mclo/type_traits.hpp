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

	template <typename T, typename... Ts>
	constexpr bool is_any_of_v = ( std::is_same_v<T, Ts> || ... );

	template <typename T>
	constexpr bool is_standard_integer = std::is_integral_v<T> && !is_any_of_v<std::remove_cv_t<T>,
																			   bool,
																			   char,
																			   wchar_t,
#ifdef __cpp_char8_t
																			   char8_t,
#endif
																			   char16_t,
																			   char32_t>;

	template <typename T>
	constexpr bool is_standard_unsigned_integer = is_any_of_v<std::remove_cv_t<T>,
															  unsigned char,
															  unsigned short,
															  unsigned int,
															  unsigned long,
															  unsigned long long>;
}
