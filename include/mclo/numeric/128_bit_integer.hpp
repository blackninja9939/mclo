#pragma once

#ifdef _MSC_VER
#include <__msvc_int128.hpp>
#endif

namespace mclo
{
#ifdef _MSC_VER
	using int128_t = std::_Signed128;
	using uint128_t = std::_Unsigned128;
#else
	using int128_t = __int128;
	using uint128_t = unsigned __int128;
#endif
}
