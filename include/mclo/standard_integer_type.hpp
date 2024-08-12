#pragma once

#include "any_of_type.hpp"

namespace mclo
{
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
