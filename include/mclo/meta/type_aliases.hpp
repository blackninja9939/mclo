#pragma once

#include "type_list.hpp"

// Not using meta::join as its got compile time overhead not needed for such simple list concatenation
#define MCLO_SIGNED_INTEGERS_LIST signed char, signed short, signed int, signed long, signed long long
#define MCLO_UNSIGNED_INTEGERS_LIST unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long
#define MCLO_FLOAT_LIST float, double, long double

namespace mclo::meta
{
	/// @brief A @ref type_list of the standard signed integer types.
	using signed_integers = type_list<MCLO_SIGNED_INTEGERS_LIST>;
	/// @brief A @ref type_list of the standard unsigned integer types.
	using unsigned_integers = type_list<MCLO_UNSIGNED_INTEGERS_LIST>;

	/// @brief A @ref type_list of all standard signed and unsigned integer types.
	using integers = type_list<MCLO_SIGNED_INTEGERS_LIST, MCLO_UNSIGNED_INTEGERS_LIST>;

	/// @brief A @ref type_list of the standard floating-point types.
	using floating_points = type_list<MCLO_FLOAT_LIST>;
	/// @brief A @ref type_list of all standard integer and floating-point types.
	using numeric_types = type_list<MCLO_SIGNED_INTEGERS_LIST, MCLO_UNSIGNED_INTEGERS_LIST, MCLO_FLOAT_LIST>;

	/// @brief A @ref type_list of the standard character types, including @c char8_t where supported.
	using char_types = type_list<char,
								 wchar_t,
#ifdef __cpp_char8_t
								 char8_t,
#endif
								 char16_t,
								 char32_t>;
}

#undef MCLO_SIGNED_INTEGERS_LIST
#undef MCLO_UNSIGNED_INTEGERS_LIST
#undef MCLO_FLOAT_LIST
