#pragma once

#include "type_list.hpp"

// Not using meta::join as its got compile time overhead not needed for such simple list concatenation
#define MCLO_SIGNED_INTEGERS_LIST signed char, signed short, signed int, signed long, signed long long
#define MCLO_UNSIGNED_INTEGERS_LIST unsigned char, unsigned short, unsigned int, unsigned long, unsigned long long
#define MCLO_FLOAT_LIST float, double, long double

namespace mclo::meta
{
	using signed_integers = type_list<MCLO_SIGNED_INTEGERS_LIST>;
	using unsigned_integers = type_list<MCLO_UNSIGNED_INTEGERS_LIST>;

	using integers = type_list<MCLO_SIGNED_INTEGERS_LIST, MCLO_UNSIGNED_INTEGERS_LIST>;

	using floating_points = type_list<MCLO_FLOAT_LIST>;
	using numeric_types = type_list<MCLO_SIGNED_INTEGERS_LIST, MCLO_UNSIGNED_INTEGERS_LIST, MCLO_FLOAT_LIST>;

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
